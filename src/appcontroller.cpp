#include "appcontroller.h"
#include "mainwindow.h"
#include "screenshotmanager.h"
#include "recognitionmanager.h"
#include "settingsmanager.h"
#include "traymanager.h"
#include "shortcuthandler.h"
#include "imageprocessor.h"
#include "settingsdialog.h"
#include "singleapplication.h"
#include "historymanager.h"
#include "floatingball.h"

#include <QApplication>
#include <QTimer>
#include <QDebug>
#include <QLocalServer>
#include <QLocalSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStatusBar>

static const QString SINGLE_INSTANCE_KEY = "hiocr_single_instance_socket";

AppController::AppController(QObject *parent) : QObject(parent) { /* ... */ }

AppController::~AppController()
{
    if (m_floatingBall) {
        delete m_floatingBall;
        m_floatingBall = nullptr;
    }
    if (m_mainWindow) {
        delete m_mainWindow;
        m_mainWindow = nullptr;
    }
}

void AppController::initialize()
{
    setupManagers();
    // 连接单实例消息
    SingleApplication* singleApp = qobject_cast<SingleApplication*>(qApp->property("singleApp").value<QObject*>());
    if (singleApp) {
        connect(singleApp, &SingleApplication::messageReceived, this, &AppController::onSingleInstanceMessageReceived);
    }
    setupConnections();
    applySettings();

    m_mainWindow->updateServiceSelector(m_settings->serviceProfiles(), m_settings->currentServiceId());

    QString currentId = m_settings->currentServiceId();
    bool isRunning = false;

    if (!currentId.isEmpty()) {
        isRunning = m_serviceManager->isServiceRunning(currentId);
        applyServiceConfig(currentId);
    } else {
        applyServiceConfig("");
    }

    m_mainWindow->updateServiceControlButton(currentId, isRunning);
    m_mainWindow->updateStopAllAction(m_serviceManager->runningCount());
}

void AppController::handleCommandLineArguments(const QString& imagePath, const QString& resultText)
{
    QImage img;
    if (!imagePath.isEmpty()) {
        img.load(imagePath);
        if (img.isNull()) {
            emit recognitionFailed("无法加载图片: " + imagePath);
            return;
        }
    }

    showWindow();

    if (!resultText.isEmpty()) {
        if (!img.isNull()) {
            emit imageChanged(img);
            QString base64 = ImageProcessor::imageToBase64(img);
            m_recognitionManager->setCurrentBase64(base64);
        }
        emit recognitionResultReady(resultText);
        return;
    }

    if (!img.isNull()) {
        emit imageChanged(img);
        if (m_settings->autoRecognizeOnScreenshot()) {
            QString base64 = ImageProcessor::imageToBase64(img);
            m_recognitionManager->onImageChanged(base64);
        } else {
            emit recognitionResultReady(QString());
        }
    }
}

MainWindow* AppController::mainWindow() const
{
    return m_mainWindow;
}

void AppController::setupManagers()
{
    m_settings = SettingsManager::instance();
    m_screenshotManager = new ScreenshotManager(this);
    m_recognitionManager = new RecognitionManager(this);
    m_mainWindow = new MainWindow();
    m_trayManager = new TrayManager(this);
    m_shortcutHandler = new ShortcutHandler(this);
    m_serviceManager = new ServiceManager(this);
    m_copyProcessor = new CopyProcessor(this);
    HistoryManager::instance()->init();

    // 【关键】传入 m_mainWindow 作为 parent
    // KDE Wayland 下，Qt::Tool 窗口有 transient parent 时，
    // KWin 才会允许 move() 生效，位置恢复才能工作
    m_floatingBall = new FloatingBall(m_mainWindow);
}



void AppController::setupConnections()
{
    // --- 1. 快捷键 -> 动作 ---
    connect(m_shortcutHandler, &ShortcutHandler::screenshotRequested, this, &AppController::takeScreenshot);
    connect(m_shortcutHandler, &ShortcutHandler::textRecognizeRequested, this, &AppController::takeTextRecognizeScreenshot);
    connect(m_shortcutHandler, &ShortcutHandler::formulaRecognizeRequested, this, &AppController::takeFormulaRecognizeScreenshot);
    connect(m_shortcutHandler, &ShortcutHandler::tableRecognizeRequested, this, &AppController::takeTableRecognizeScreenshot);

    // 【新增】连接分类型处理脚本快捷键
    connect(m_shortcutHandler, &ShortcutHandler::textProcessorRequested, this, [this](){
        onManualProcessorTriggered(ContentType::Text);
    });
    connect(m_shortcutHandler, &ShortcutHandler::formulaProcessorRequested, this, [this](){
        onManualProcessorTriggered(ContentType::Formula);
    });
    connect(m_shortcutHandler, &ShortcutHandler::tableProcessorRequested, this, [this](){
        onManualProcessorTriggered(ContentType::Table);
    });
    connect(m_shortcutHandler, &ShortcutHandler::pureMathProcessorRequested, this, [this](){
        onManualProcessorTriggered(ContentType::PureMath);
    });

    // --- 2. 托盘 -> 动作 ---
    connect(m_trayManager, &TrayManager::screenshotRequested, this, &AppController::takeScreenshot);
    connect(m_trayManager, &TrayManager::textRecognizeRequested, this, &AppController::takeTextRecognizeScreenshot);
    connect(m_trayManager, &TrayManager::formulaRecognizeRequested, this, &AppController::takeFormulaRecognizeScreenshot);
    connect(m_trayManager, &TrayManager::tableRecognizeRequested, this, &AppController::takeTableRecognizeScreenshot);
    connect(m_trayManager, &TrayManager::showWindowRequested, this, &AppController::showWindow);
    connect(m_trayManager, &TrayManager::quitRequested, this, &AppController::quitApp);

    // --- 3. MainWindow UI -> Controller ---
    connect(m_mainWindow, &MainWindow::recognizeRequested, this, [this](const QString& prompt, const QString& base64Img){
        ContentType inferredType = ContentType::Text;
        QString currentId = m_settings->currentServiceId();
        ServiceProfile p = m_settings->getServiceProfile(currentId);
        QString formulaP = p.isEmpty() ? m_settings->formulaPrompt() : p.formulaPrompt;
        QString tableP = p.isEmpty() ? m_settings->tablePrompt() : p.tablePrompt;

        if (prompt == formulaP) inferredType = ContentType::Formula;
        else if (prompt == tableP) inferredType = ContentType::Table;

        m_lastRecognizeType = inferredType;
        m_recognitionManager->recognize(prompt, base64Img);
    });
    connect(m_mainWindow, &MainWindow::typedRecognizeRequested, this, [this](const QString& prompt, const QString& base64, ContentType type){
        m_lastRecognizeType = type;
        m_recognitionManager->recognize(prompt, base64);
    });

    connect(m_mainWindow, &MainWindow::imagePasted, this, [this](const QImage& img){
        m_mainWindow->setImage(img);
        m_recognitionManager->onImageChanged(ImageProcessor::imageToBase64(img));
    });

    connect(m_mainWindow, &MainWindow::areaSelected, this, &AppController::onAreaSelected);
    connect(m_mainWindow, &MainWindow::screenshotRequested, this, &AppController::takeScreenshot);
    connect(m_mainWindow, &MainWindow::stopServiceRequested, m_serviceManager, &ServiceManager::stopAllServices);
    connect(m_mainWindow, &MainWindow::settingsTriggered, this, [this](){
        SettingsDialog dialog(m_mainWindow);
        dialog.exec();
    });

    // 服务控制
    connect(m_mainWindow, &MainWindow::serviceSelected, this, &AppController::onServiceSelected);
    connect(m_mainWindow, &MainWindow::serviceToggleRequested, this, &AppController::toggleService);

    // 处理来自 MainWindow 的手动脚本处理请求
    connect(m_mainWindow, &MainWindow::manualProcessRequested, this, &AppController::onManualProcessorTriggered);

    // --- 4. ScreenshotManager ---
    connect(m_screenshotManager, &ScreenshotManager::screenshotCaptured, this, &AppController::onScreenshotCaptured);
    connect(m_screenshotManager, &ScreenshotManager::screenshotFailed, this, &AppController::onScreenshotFailed);

    // --- 5. RecognitionManager -> Controller ---

    // 连接流式数据信号 -> 追加 UI
    connect(m_recognitionManager, &RecognitionManager::streamDataReceived, m_mainWindow, &MainWindow::appendRecognitionResult);

    // 处理状态变化：识别开始时清空 UI
    connect(m_recognitionManager, &RecognitionManager::busyStateChanged, this, [this](bool busy){
        if (busy) {
            emit recognitionResultReady(QString());
            m_mainWindow->setStreamingMode(true);

            // 【新增】静默模式：显示"识别中"通知
            if (m_settings->silentModeEnabled() && !m_mainWindow->isVisible()) {
                showSilentNotification(FloatingBall::Recognizing, "正在识别...");
            }
        } else {
            m_mainWindow->setStreamingMode(false);
        }
        emit busyStateChanged(busy);
    });


    // 【新增】处理历史记录加载请求
    connect(m_mainWindow, &MainWindow::loadHistoryRecordRequested, this, [this](int recordId){
        HistoryRecord record = HistoryManager::instance()->getRecordById(recordId);
        if (record.id != -1) {
            // 1. 加载图片
            QImage img(record.cachedImagePath);
            if (!img.isNull()) {
                m_mainWindow->setImage(img);
                // 注意：这里要更新 RecognitionManager 的 Base64 缓存，否则立即点击重新识别会出错
                m_recognitionManager->setCurrentBase64(ImageProcessor::imageToBase64(img));
            }

            // 2. 加载结果文本
            m_mainWindow->setRecognitionResult(record.resultText);
            m_mainWindow->setRecognizeType(record.recognitionType);

            // 3. 更新提示词下拉框和编辑框
            QString prompt;
            // ... (复用您原有的获取 prompt 逻辑) ...
            // 简单示例：
            switch(record.recognitionType) {
                case ContentType::Formula: prompt = m_settings->formulaPrompt(); break;
                case ContentType::Table: prompt = m_settings->tablePrompt(); break;
                default: prompt = m_settings->textPrompt();
            }
            m_mainWindow->setPrompt(prompt);

            showWindow(); // 激活窗口
        }
    });

    // 处理识别完成
    connect(m_recognitionManager, &RecognitionManager::recognitionFinished, this, [this](const QString& markdown){
        m_isRetryingAfterSwitch = false;
        m_serviceManager->resetIdleTimer();

        if (!markdown.isEmpty()) {
            emit recognitionResultReady(markdown);
        }

        QString finalText = m_mainWindow->currentMarkdownSource();
        m_mainWindow->setRecognizeType(m_lastRecognizeType);

        // 【新增】保存历史记录
        if (m_settings->saveHistoryEnabled()) {
            // 直接从 MainWindow 获取当前显示的图片
            QImage currentImage = m_mainWindow->currentImage();

            if (!currentImage.isNull() && !finalText.isEmpty()) {
                HistoryManager::instance()->saveRecord(currentImage, finalText, m_lastRecognizeType);
            }
        }

        if (m_settings->autoCopyResult() && !finalText.isEmpty()) {
            m_copyProcessor->processAndCopy(finalText, m_lastRecognizeType);
        }

        // 【新增】静默模式：显示"识别完成"通知
        if (m_settings->silentModeEnabled() && !m_mainWindow->isVisible()) {
            showSilentNotification(FloatingBall::Success, "识别完成");
        }
    });

    // 识别失败处理逻辑
    connect(m_recognitionManager, &RecognitionManager::recognitionFailed, this, [this](const QString& error){
        // 确保出错时也关闭流式模式
        m_mainWindow->setStreamingMode(false);

        if (m_isRetryingAfterSwitch) {
            m_isRetryingAfterSwitch = false;
            emit recognitionFailed(error);
            if (m_settings->silentModeEnabled() && !m_mainWindow->isVisible()) {
                showSilentNotification(FloatingBall::Error, "识别错误: " + error);
            }
            return;
        }

        bool isNetworkError = error.contains("Connection refused") ||
        error.contains("连接被拒绝") ||
        error.contains("Network error") ||
        error.contains("网络错误") ||
        error.contains("Host not found") ||
        error.contains("Connection closed") ||
        error.contains("Timeout") ||
        error.contains("unreachable");

        if (!isNetworkError) {
            if (m_settings->silentModeEnabled() && !m_mainWindow->isVisible()) {
                showSilentNotification(FloatingBall::Error, "识别错误: " + error);
            }
            emit recognitionFailed(error);
            return;
        }

        // --- 连接失败后的自动恢复逻辑 ---
        QString currentId = m_settings->currentServiceId();
        bool isServiceMode = !currentId.isEmpty();

        bool canTryRecover = false;
        QString tryStartId;
        QString tryStartCommand;

        if (isServiceMode) {
            ServiceProfile p = m_settings->getServiceProfile(currentId);
            if (!p.startCommand.isEmpty()) {
                canTryRecover = true;
                tryStartId = currentId;
                tryStartCommand = p.startCommand;
                emit recognitionFailed("服务连接失败，正在尝试启动服务...");
            }
        } else {
            if (m_settings->autoStartService()) {
                QString defaultLocalId = m_settings->defaultLocalServiceId();
                if (!defaultLocalId.isEmpty()) {
                    ServiceProfile p = m_settings->getServiceProfile(defaultLocalId);
                    if (!p.startCommand.isEmpty()) {
                        canTryRecover = true;
                        tryStartId = defaultLocalId;
                        tryStartCommand = p.startCommand;

                        m_settings->setCurrentServiceId(defaultLocalId);
                        m_mainWindow->updateServiceSelector(m_settings->serviceProfiles(), defaultLocalId);

                        emit recognitionFailed("全局连接失败，正在切换并启动本地服务...");
                    }
                }
            }
        }

        if (canTryRecover) {
            m_retryAfterServiceStart = true;
            m_isRetryingAfterSwitch = true;
            m_pendingPrompt = m_recognitionManager->lastPrompt();
            m_pendingBase64 = m_recognitionManager->currentBase64();

            applyServiceConfig(tryStartId);

            if (m_settings->serviceSwitchMode() == SettingsManager::SingleService) {
                m_serviceManager->stopAllServices();
            }
            m_serviceManager->startService(tryStartId, tryStartCommand);
        } else {
            if (m_settings->silentModeEnabled() && !m_mainWindow->isVisible()) {
                showSilentNotification(FloatingBall::Error, "识别错误: " + error);
            }
            emit recognitionFailed(error);
        }
    });

    // --- 6. Controller -> MainWindow ---
    connect(this, &AppController::imageChanged, m_mainWindow, &MainWindow::setImage);
    connect(this, &AppController::recognitionResultReady, m_mainWindow, &MainWindow::setRecognitionResult);
    connect(this, &AppController::recognitionFailed, m_mainWindow, &MainWindow::showError);

    // 【关键修正】添加这一行：连接 Busy 信号到 MainWindow 的 setBusy 槽
    connect(this, &AppController::busyStateChanged, m_mainWindow, &MainWindow::setBusy);

    connect(this, &AppController::requestAreaSelection, m_mainWindow, &MainWindow::startAreaSelection);

    // --- 7. 设置变更 ---
    connect(m_settings, &SettingsManager::shortcutsChanged, this, &AppController::onSettingsChanged);
    connect(m_settings, &SettingsManager::autoUseLastPromptChanged, m_recognitionManager, &RecognitionManager::setAutoUseLastPrompt);

    connect(m_settings, &SettingsManager::serviceProfilesChanged, this, [this](){
        m_mainWindow->updateServiceSelector(m_settings->serviceProfiles(), m_settings->currentServiceId());
        QString currentId = m_settings->currentServiceId();
        if (!currentId.isEmpty()) {
            applyServiceConfig(currentId);
        }
    });

    // --- 8. ServiceManager ---
    connect(m_serviceManager, &ServiceManager::serviceStarted, this, [this](const QString& id){
        m_mainWindow->updateServiceControlButton(id, true);
        m_mainWindow->updateStopAllAction(m_serviceManager->runningCount());

        if (m_retryAfterServiceStart && m_settings->currentServiceId() == id) {
            QTimer::singleShot(2000, this, [this](){
                if (!m_pendingBase64.isEmpty() && !m_pendingPrompt.isEmpty()) {
                    applyServiceConfig(m_settings->currentServiceId());
                    m_recognitionManager->recognize(m_pendingPrompt, m_pendingBase64);
                    m_pendingBase64.clear();
                    m_pendingPrompt.clear();
                }
                m_retryAfterServiceStart = false;
            });
        }
    });

    connect(m_serviceManager, &ServiceManager::serviceStopped, this, [this](const QString& id){
        m_mainWindow->updateServiceControlButton(id, false);
        m_mainWindow->updateStopAllAction(m_serviceManager->runningCount());
    });

    connect(m_serviceManager, &ServiceManager::serviceError, this, [this](const QString& id, const QString& error){
        Q_UNUSED(id);
        m_isRetryingAfterSwitch = false;
        emit recognitionFailed("服务错误: " + error);
    });

    connect(m_serviceManager, &ServiceManager::runningCountChanged, m_mainWindow, &MainWindow::updateStopAllAction);

    // 静默模式被关闭时，隐藏悬浮球
    connect(m_settings, &SettingsManager::silentModeEnabledChanged, this, [this](bool enabled){
        if (!enabled && m_floatingBall) {
            m_floatingBall->setState(FloatingBall::Idle);
        }
    });

    // --- 悬浮球交互 ---
    connect(m_floatingBall, &FloatingBall::screenshotTriggered, this, &AppController::takeScreenshot);
    connect(m_floatingBall, &FloatingBall::showWindowTriggered, this, &AppController::onSilentNotificationClicked);
    connect(m_floatingBall, &FloatingBall::positionChanged, this, &AppController::onFloatingBallPositionChanged);

    // 设置实时应用
    connect(m_settings, &SettingsManager::floatingBallSizeChanged, this, &AppController::onFloatingBallSettingsChanged);
    connect(m_settings, &SettingsManager::floatingBallAutoHideTimeChanged, this, &AppController::onFloatingBallSettingsChanged);
    connect(m_settings, &SettingsManager::floatingBallAlwaysVisibleChanged, this, &AppController::onFloatingBallSettingsChanged);

    // --- 9. CopyProcessor 状态反馈 ---
    connect(m_copyProcessor, &CopyProcessor::error, m_mainWindow, &MainWindow::showError);
    connect(m_copyProcessor, &CopyProcessor::finished, this, [this](const QString& result){
        Q_UNUSED(result);
        m_mainWindow->statusBar()->showMessage("内容已处理并复制", 3000);
    });
}


void AppController::onServiceSelected(const QString& id)
{
    if (m_settings->serviceSwitchMode() == SettingsManager::SingleService) {
        if (!m_serviceManager->isServiceRunning(id)) {
            m_serviceManager->stopAllServices();
        }
    }
    m_settings->setCurrentServiceId(id);

    bool isRunning = m_serviceManager->isServiceRunning(id);
    m_mainWindow->updateServiceControlButton(id, isRunning);

    applyServiceConfig(id);
}

void AppController::toggleService(const QString& id)
{
    if (id.isEmpty()) return;

    if (m_serviceManager->isServiceRunning(id)) {
        m_serviceManager->stopService(id);
    } else {
        ServiceProfile p = m_settings->getServiceProfile(id);
        if (p.startCommand.isEmpty()) {
            emit recognitionFailed("该服务未配置启动命令（可能是远程服务），无法启动。");
            return;
        }

        // 【关键】确保在启动服务前，应用该服务的网络配置
        applyServiceConfig(id);
        if (m_settings->currentServiceId() != id) {
            m_settings->setCurrentServiceId(id);
            m_mainWindow->updateServiceSelector(m_settings->serviceProfiles(), id);
        }

        if (m_settings->serviceSwitchMode() == SettingsManager::SingleService) {
            m_serviceManager->stopAllServices();
        }
        m_serviceManager->startService(id, p.startCommand);
    }
}

void AppController::applyServiceConfig(const QString& serviceId)
{
    QString textP, formulaP, tableP, url, apiKey, modelName, serviceName;

    if (serviceId.isEmpty()) {
        // 使用全局默认设置
        textP = m_settings->textPrompt();
        formulaP = m_settings->formulaPrompt();
        tableP = m_settings->tablePrompt();
        url = m_settings->serverUrl();
        apiKey = m_settings->globalApiKey();
        modelName = m_settings->globalModelName();
        serviceName = QString(); // 全局默认无服务名称
    } else {
        // 使用特定服务配置
        ServiceProfile p = m_settings->getServiceProfile(serviceId);
        if (p.isEmpty()) {
            textP = m_settings->textPrompt();
            formulaP = m_settings->formulaPrompt();
            tableP = m_settings->tablePrompt();
            url = m_settings->serverUrl();
            apiKey = m_settings->globalApiKey();
            modelName = m_settings->globalModelName();
            serviceName = QString();
        } else {
            textP = p.textPrompt;
            formulaP = p.formulaPrompt;
            tableP = p.tablePrompt;
            url = p.serverUrl;
            apiKey = p.apiKey;
            modelName = p.modelName;
            serviceName = p.name; // 【新增】获取服务名称
        }
    }

    m_recognitionManager->setServerUrl(url);
    m_recognitionManager->setApiKey(apiKey);
    m_recognitionManager->setModelName(modelName);

    // 【新增】保存当前服务名称并同步到所有组件
    m_currentServiceName = serviceName;

    // 同步到 MainWindow（渲染器 + 复制栏）
    m_mainWindow->setCurrentServiceName(serviceName);

    // 同步到 AppController 自己的 CopyProcessor（自动复制路径）
    if (m_copyProcessor) {
        m_copyProcessor->setServiceName(serviceName);
    }

    qDebug() << "Applied service config. Name:" << serviceName
    << "URL:" << url << "Model:" << modelName;

    m_mainWindow->setCurrentPrompts(textP, formulaP, tableP);
    m_mainWindow->setPrompt(textP);
}

void AppController::takeScreenshot() {
    m_pendingPromptOverride = "";

    if (m_floatingBall) {
        // 截图前隐藏，showEvent 中会尝试恢复位置
        m_floatingBall->hide();
    }

    m_screenshotManager->takeScreenshot();
}

void AppController::takeTextRecognizeScreenshot() {
    m_lastRecognizeType = ContentType::Text;
    QString currentId = m_settings->currentServiceId();
    QString prompt;
    if (currentId.isEmpty()) prompt = m_settings->textPrompt();
    else { ServiceProfile p = m_settings->getServiceProfile(currentId); prompt = p.isEmpty() ? m_settings->textPrompt() : p.textPrompt; }

    m_pendingPromptOverride = prompt;
    m_mainWindow->setPrompt(prompt);

    if (m_floatingBall) {
        m_floatingBall->hide();
    }

    m_screenshotManager->takeScreenshot();
}

void AppController::takeFormulaRecognizeScreenshot() {
    m_lastRecognizeType = ContentType::Formula;
    QString currentId = m_settings->currentServiceId();
    QString prompt;
    if (currentId.isEmpty()) prompt = m_settings->formulaPrompt();
    else { ServiceProfile p = m_settings->getServiceProfile(currentId); prompt = p.isEmpty() ? m_settings->formulaPrompt() : p.formulaPrompt; }

    m_pendingPromptOverride = prompt;
    m_mainWindow->setPrompt(prompt);

    if (m_floatingBall) {
        m_floatingBall->hide();
    }

    m_screenshotManager->takeScreenshot();
}

void AppController::takeTableRecognizeScreenshot() {
    m_lastRecognizeType = ContentType::Table;
    QString currentId = m_settings->currentServiceId();
    QString prompt;
    if (currentId.isEmpty()) prompt = m_settings->tablePrompt();
    else { ServiceProfile p = m_settings->getServiceProfile(currentId); prompt = p.isEmpty() ? m_settings->tablePrompt() : p.tablePrompt; }

    m_pendingPromptOverride = prompt;
    m_mainWindow->setPrompt(prompt);

    if (m_floatingBall) {
        m_floatingBall->hide();
    }

    m_screenshotManager->takeScreenshot();
}

void AppController::showWindow() {
    if (m_mainWindow) {
        m_mainWindow->show();
        m_mainWindow->raise();
        m_mainWindow->activateWindow();
    }
    // 【新增】手动打开主窗口时，隐藏悬浮球
    if (m_floatingBall && m_floatingBall->currentState() != FloatingBall::Idle) {
        m_floatingBall->setState(FloatingBall::Idle);
    }
}

void AppController::quitApp() {
    SettingsManager::instance()->sync();
    QApplication::quit();
}

void AppController::onScreenshotCaptured(const QImage& image) {
    m_pendingFullScreenshot = image;
    emit requestAreaSelection(image);
}

void AppController::onScreenshotFailed(const QString& error) {
    // 截图失败，恢复悬浮球显示
    if (m_floatingBall && m_settings->floatingBallAlwaysVisible()) {
        m_floatingBall->setState(FloatingBall::Idle);
    }

    if (m_mainWindow && m_mainWindow->isVisible()) {
        emit recognitionFailed(error);
    }
}

void AppController::onAreaSelected(const QRect& rect) {
    if (m_pendingFullScreenshot.isNull()) {
        if (m_floatingBall && m_settings->floatingBallAlwaysVisible()) {
            m_floatingBall->setState(FloatingBall::Idle);
        }
        return;
    }

    if (rect.isNull() || rect.isEmpty()) {
        m_pendingFullScreenshot = QImage();
        if (m_floatingBall && m_settings->floatingBallAlwaysVisible()) {
            m_floatingBall->setState(FloatingBall::Idle);
        }
        return;
    }

    QImage cropped = m_pendingFullScreenshot.copy(rect);
    m_pendingFullScreenshot = QImage();

    emit imageChanged(cropped);

    if (m_settings->autoRecognizeOnScreenshot()) {
        QString base64 = ImageProcessor::imageToBase64(cropped);
        if (!m_pendingPromptOverride.isEmpty()) {
            m_recognitionManager->setTempPromptOverride(m_pendingPromptOverride);
            m_pendingPromptOverride.clear();
        }
        m_recognitionManager->onImageChanged(base64);
    } else {
        emit recognitionResultReady(QString());
        if (m_floatingBall && m_settings->floatingBallAlwaysVisible()) {
            m_floatingBall->setState(FloatingBall::Idle);
        }
    }

    QTimer::singleShot(0, this, [this](){
        if (!m_settings->silentModeEnabled()) {
            showWindow();
        }
    });
}

void AppController::onSettingsChanged() {
    if (m_shortcutHandler) {
        m_shortcutHandler->applyShortcuts();
    }
}

void AppController::applySettings()
{
    if (m_recognitionManager && m_settings) {
        m_recognitionManager->setAutoUseLastPrompt(m_settings->autoUseLastPrompt());
    }
}


void AppController::onSingleInstanceMessageReceived(const QByteArray& message)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(message, &err);
    if (err.error == QJsonParseError::NoError && doc.isObject()) {
        QJsonObject obj = doc.object();
        QString image = obj["image"].toString();
        QString result = obj["result"].toString();
        handleCommandLineArguments(image, result);
    }
}


// 【新增】手动触发脚本处理的槽函数
void AppController::onManualProcessorTriggered(ContentType type)
{
    // 从 MainWindow 获取当前显示的结果文本
    QString currentText = m_mainWindow->currentMarkdownSource();
    if (currentText.isEmpty()) {
        m_mainWindow->statusBar()->showMessage("内容为空，无法处理。", 3000);
        return;
    }

    // 调用 CopyProcessor 的手动模式
    m_copyProcessor->manualProcess(currentText, type);
}



void AppController::showSilentNotification(FloatingBall::State state, const QString& message)
{
    if (!m_settings->silentModeEnabled()) return;

    QString notificationType = m_settings->silentModeNotificationType();

    if (notificationType == "floating_ball" && m_floatingBall) {
        m_floatingBall->setState(state, message);
    } else {
        // 系统通知模式
        QString title = "HiOCR";
        switch (state) {
            case FloatingBall::Recognizing:
                m_trayManager->showMessage(title, "正在识别...");
                break;
            case FloatingBall::Success:
                m_trayManager->showMessage(title, "识别完成");
                break;
            case FloatingBall::Error:
                m_trayManager->showMessage(title, message.isEmpty() ? "识别错误" : message);
                break;
            default:
                break;
        }
    }
}

void AppController::onSilentNotificationClicked()
{
    // 点击通知或悬浮球时，打开主窗口查看结果
    showWindow();
}

void AppController::onFloatingBallPositionChanged(const QPoint& pos)
{
    // 所有平台都保存位置（悬浮球现在通过光标位移追踪，Wayland 下也能得到正确坐标）
    m_settings->setFloatingBallPosX(pos.x());
    m_settings->setFloatingBallPosY(pos.y());
}

void AppController::onFloatingBallSettingsChanged()
{
    // 将设置实时应用到悬浮球
    if (m_floatingBall) {
        QPoint pos(m_settings->floatingBallPosX(), m_settings->floatingBallPosY());
        m_floatingBall->applySettings(
            m_settings->floatingBallSize(),
                                      pos,
                                      m_settings->floatingBallAutoHideTime(),
                                      m_settings->floatingBallAlwaysVisible()
        );
    }
}
