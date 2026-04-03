#include "appcontroller.h"
#include "mainwindow.h"
#include "screenshotmanager.h"
#include "recognitionmanager.h"
#include "settingsmanager.h"
#include "traymanager.h"
#include "shortcuthandler.h"
#include "imageprocessor.h"
#include "settingsdialog.h"
#include "singleapplication.h" // 【新增】

#include <QApplication>
#include <QTimer>
#include <QDebug>
#include <QLocalServer>
#include <QLocalSocket>
#include <QJsonDocument>
#include <QJsonObject>

static const QString SINGLE_INSTANCE_KEY = "hiocr_single_instance_socket";

AppController::AppController(QObject *parent) : QObject(parent) { /* ... */ }

AppController::~AppController()
{
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
}


void AppController::setupConnections()
{
    // 快捷键 -> 动作
    connect(m_shortcutHandler, &ShortcutHandler::screenshotRequested, this, &AppController::takeScreenshot);
    connect(m_shortcutHandler, &ShortcutHandler::textRecognizeRequested, this, &AppController::takeTextRecognizeScreenshot);
    connect(m_shortcutHandler, &ShortcutHandler::formulaRecognizeRequested, this, &AppController::takeFormulaRecognizeScreenshot);
    connect(m_shortcutHandler, &ShortcutHandler::tableRecognizeRequested, this, &AppController::takeTableRecognizeScreenshot);
    connect(m_shortcutHandler, &ShortcutHandler::externalProcessRequested, m_mainWindow, &MainWindow::onExternalProcessTriggered);

    // 托盘 -> 动作
    connect(m_trayManager, &TrayManager::screenshotRequested, this, &AppController::takeScreenshot);
    connect(m_trayManager, &TrayManager::textRecognizeRequested, this, &AppController::takeTextRecognizeScreenshot);
    connect(m_trayManager, &TrayManager::formulaRecognizeRequested, this, &AppController::takeFormulaRecognizeScreenshot);
    connect(m_trayManager, &TrayManager::tableRecognizeRequested, this, &AppController::takeTableRecognizeScreenshot);
    connect(m_trayManager, &TrayManager::showWindowRequested, this, &AppController::showWindow);
    connect(m_trayManager, &TrayManager::quitRequested, this, &AppController::quitApp);

    // MainWindow UI -> Controller
    // 【修复】直接调用 RecognitionManager，不要尝试在这里组装网络请求
    connect(m_mainWindow, &MainWindow::recognizeRequested, this, [this](const QString& prompt, const QString& base64Img){
        m_recognitionManager->recognize(prompt, base64Img);
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

    // ScreenshotManager
    connect(m_screenshotManager, &ScreenshotManager::screenshotCaptured, this, &AppController::onScreenshotCaptured);
    connect(m_screenshotManager, &ScreenshotManager::screenshotFailed, this, &AppController::onScreenshotFailed);

    // RecognitionManager -> Controller
    connect(m_recognitionManager, &RecognitionManager::recognitionFinished, this, [this](const QString& markdown){
        m_isRetryingAfterSwitch = false;
        m_serviceManager->resetIdleTimer();
        onRecognitionFinished(markdown);
    });

    // 【核心重构】识别失败处理逻辑
    connect(m_recognitionManager, &RecognitionManager::recognitionFailed, this, [this](const QString& error){
        // 如果已经是重试后的失败，直接报错并重置标志，不再循环尝试
        if (m_isRetryingAfterSwitch) {
            m_isRetryingAfterSwitch = false;
            emit recognitionFailed(error);
            return;
        }

        // 非连接类错误，直接报错
        if (!error.contains("Connection refused") && !error.contains("连接被拒绝")) {
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
            // 当前是服务模式：尝试启动当前选中的服务
            ServiceProfile p = m_settings->getServiceProfile(currentId);
            qDebug() << "Connection failed. Service ID:" << currentId << "URL:" << p.serverUrl;

            if (!p.startCommand.isEmpty()) {
                canTryRecover = true;
                tryStartId = currentId;
                tryStartCommand = p.startCommand;
                emit recognitionFailed("服务连接失败，正在尝试启动服务...");
            }
        } else {
            // 当前是全局模式：检查是否开启了自动启动本地服务
            if (m_settings->autoStartService()) {
                QString defaultLocalId = m_settings->defaultLocalServiceId();
                if (!defaultLocalId.isEmpty()) {
                    ServiceProfile p = m_settings->getServiceProfile(defaultLocalId);
                    if (!p.startCommand.isEmpty()) {
                        canTryRecover = true;
                        tryStartId = defaultLocalId;
                        tryStartCommand = p.startCommand;
                        // 自动切换到该服务
                        m_settings->setCurrentServiceId(defaultLocalId);
                        emit recognitionFailed("全局连接失败，正在切换并启动本地服务...");
                    }
                }
            }
        }

        if (canTryRecover) {
            m_retryAfterServiceStart = true;
            m_isRetryingAfterSwitch = true; // 标记进入重试流程
            m_pendingPrompt = m_recognitionManager->lastPrompt();
            m_pendingBase64 = m_recognitionManager->currentBase64();

            // 在启动服务前，强制更新 RecognitionManager 的 URL
            applyServiceConfig(tryStartId);

            if (m_settings->serviceSwitchMode() == SettingsManager::SingleService) {
                m_serviceManager->stopAllServices();
            }
            m_serviceManager->startService(tryStartId, tryStartCommand);
        } else {
            emit recognitionFailed(error);
        }
    });

    connect(m_recognitionManager, &RecognitionManager::busyStateChanged, this, &AppController::busyStateChanged);

    // Controller -> MainWindow
    connect(this, &AppController::imageChanged, m_mainWindow, &MainWindow::setImage);
    connect(this, &AppController::recognitionResultReady, m_mainWindow, &MainWindow::setRecognitionResult);
    connect(this, &AppController::recognitionFailed, m_mainWindow, &MainWindow::showError);
    connect(this, &AppController::busyStateChanged, m_mainWindow, &MainWindow::setBusy);
    connect(this, &AppController::requestAreaSelection, m_mainWindow, &MainWindow::startAreaSelection);

    // 设置变更
    connect(m_settings, &SettingsManager::shortcutsChanged, this, &AppController::onSettingsChanged);
    connect(m_settings, &SettingsManager::autoUseLastPromptChanged, m_recognitionManager, &RecognitionManager::setAutoUseLastPrompt);

    // 【新增】监听服务列表配置变化
    connect(m_settings, &SettingsManager::serviceProfilesChanged, this, [this](){
        m_mainWindow->updateServiceSelector(m_settings->serviceProfiles(), m_settings->currentServiceId());
        QString currentId = m_settings->currentServiceId();
        if (!currentId.isEmpty()) {
            applyServiceConfig(currentId);
        }
    });

    // ServiceManager
    connect(m_serviceManager, &ServiceManager::serviceStarted, this, [this](const QString& id){
        m_mainWindow->updateServiceControlButton(id, true);
        m_mainWindow->updateStopAllAction(m_serviceManager->runningCount());

        if (m_retryAfterServiceStart && m_settings->currentServiceId() == id) {
            qDebug() << "Service started, retrying recognition in 2 seconds...";
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
    QString textP, formulaP, tableP, url;

    if (serviceId.isEmpty()) {
        textP = m_settings->textPrompt();
        formulaP = m_settings->formulaPrompt();
        tableP = m_settings->tablePrompt();
        url = m_settings->serverUrl();
    } else {
        ServiceProfile p = m_settings->getServiceProfile(serviceId);
        if (p.isEmpty()) {
            textP = m_settings->textPrompt();
            formulaP = m_settings->formulaPrompt();
            tableP = m_settings->tablePrompt();
            url = m_settings->serverUrl();
        } else {
            textP = p.textPrompt;
            formulaP = p.formulaPrompt;
            tableP = p.tablePrompt;
            url = p.serverUrl;
        }
    }

    // 【修改】调用 RecognitionManager 的接口来更新 URL
    m_recognitionManager->setServerUrl(url);

    qDebug() << "Applied service config. URL set to:" << url;

    m_mainWindow->setCurrentPrompts(textP, formulaP, tableP);
    m_mainWindow->setPrompt(textP);
}

void AppController::takeScreenshot() {
    m_pendingPromptOverride = "";
    m_screenshotManager->takeScreenshot();
}

void AppController::takeTextRecognizeScreenshot() {
    QString currentId = m_settings->currentServiceId();
    QString prompt;
    if (currentId.isEmpty()) {
        prompt = m_settings->textPrompt();
    } else {
        ServiceProfile p = m_settings->getServiceProfile(currentId);
        prompt = p.isEmpty() ? m_settings->textPrompt() : p.textPrompt;
    }

    m_pendingPromptOverride = prompt;
    m_mainWindow->setPrompt(prompt);
    m_screenshotManager->takeScreenshot();
}

void AppController::takeFormulaRecognizeScreenshot() {
    QString currentId = m_settings->currentServiceId();
    QString prompt;
    if (currentId.isEmpty()) {
        prompt = m_settings->formulaPrompt();
    } else {
        ServiceProfile p = m_settings->getServiceProfile(currentId);
        prompt = p.isEmpty() ? m_settings->formulaPrompt() : p.formulaPrompt;
    }

    m_pendingPromptOverride = prompt;
    m_mainWindow->setPrompt(prompt);
    m_screenshotManager->takeScreenshot();
}

void AppController::takeTableRecognizeScreenshot() {
    QString currentId = m_settings->currentServiceId();
    QString prompt;
    if (currentId.isEmpty()) {
        prompt = m_settings->tablePrompt();
    } else {
        ServiceProfile p = m_settings->getServiceProfile(currentId);
        prompt = p.isEmpty() ? m_settings->tablePrompt() : p.tablePrompt;
    }

    m_pendingPromptOverride = prompt;
    m_mainWindow->setPrompt(prompt);
    m_screenshotManager->takeScreenshot();
}

void AppController::showWindow() {
    if (m_mainWindow) {
        m_mainWindow->show();
        m_mainWindow->raise();
        m_mainWindow->activateWindow();
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
    if (m_mainWindow && m_mainWindow->isVisible()) {
        emit recognitionFailed(error);
    }
}

void AppController::onAreaSelected(const QRect& rect) {
    if (m_pendingFullScreenshot.isNull()) return;

    if (rect.isNull() || rect.isEmpty()) {
        m_pendingFullScreenshot = QImage();
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
    }

    QTimer::singleShot(0, this, [this](){
        showWindow();
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

void AppController::onRecognitionFinished(const QString& markdown)
{
    emit recognitionResultReady(markdown);

    if (m_settings->autoCopyResult()) {
        if (!markdown.isEmpty()) {
            m_mainWindow->copyToClipboard(markdown);
        }
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
