#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include "servicemanager.h"
#include "copyprocessor.h"
#include "floatingball.h"

#include <QObject>
#include <QImage>
#include <QApplication>
#include <QTimer>
#include <QDebug>
#include <QLocalServer>
#include <QLocalSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStatusBar>

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

class MainWindow;
class ScreenshotManager;
class RecognitionManager;
#include "settingsmanager.h"

class AppController : public QObject
{
    Q_OBJECT

public:
    explicit AppController(QObject *parent = nullptr);
    ~AppController();

    void initialize();
    MainWindow* mainWindow() const;
    void handleCommandLineArguments(const QString& imagePath, const QString& resultText);

public slots:
    void takeScreenshot();
    void takeTextRecognizeScreenshot();
    void takeFormulaRecognizeScreenshot();
    void takeTableRecognizeScreenshot();
    void abortRecognition();
    void showWindow();
    void quitApp();

signals:
    void imageChanged(const QImage& image);
    void recognitionResultReady(const QString& markdown);
    void recognitionFailed(const QString& error);
    void busyStateChanged(bool busy);
    void requestAreaSelection(const QImage& fullImage);

private slots:
    void onScreenshotCaptured(const QImage& image);
    void onScreenshotFailed(const QString& error);
    void onAreaSelected(const QRect& rect);
    void onSettingsChanged();

    void onServiceSelected(const QString& id);
    void toggleService(const QString& id);

    void onSingleInstanceMessageReceived(const QByteArray& message);
    void onManualProcessorTriggered(ContentType type);
    void onSilentNotificationClicked();

    void onFloatingBallPositionChanged(const QPoint& pos);
    void onFloatingBallSettingsChanged();

private:
    void setupManagers();
    void setupConnections();
    void applySettings();
    void setupSingleInstance();
    void applyServiceConfig(const QString& serviceId);

    MainWindow* m_mainWindow = nullptr;
    ScreenshotManager* m_screenshotManager = nullptr;
    RecognitionManager* m_recognitionManager = nullptr;
    SettingsManager* m_settings = nullptr;
    TrayManager* m_trayManager = nullptr;
    ShortcutHandler* m_shortcutHandler = nullptr;
    ServiceManager* m_serviceManager = nullptr;
    CopyProcessor* m_copyProcessor = nullptr;

    QImage m_pendingFullScreenshot;
    QString m_pendingPromptOverride;

    QString m_pendingPrompt;
    QString m_pendingBase64;
    bool m_retryAfterServiceStart = false;
    bool m_isRetryingAfterSwitch = false;

    ContentType m_lastRecognizeType = ContentType::Text;

    QString m_currentServiceName;

    void showSilentNotification(FloatingBall::State state, const QString& message = QString());

    FloatingBall* m_floatingBall = nullptr;
};

inline AppController::AppController(QObject *parent) : QObject(parent) { /* ... */ }

inline AppController::~AppController()
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

inline void AppController::initialize()
{
    setupManagers();
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

inline void AppController::handleCommandLineArguments(const QString& imagePath, const QString& resultText)
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

inline MainWindow* AppController::mainWindow() const
{
    return m_mainWindow;
}

inline void AppController::setupManagers()
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

    m_floatingBall = new FloatingBall(m_mainWindow);
}

inline void AppController::setupConnections()
{
    connect(m_shortcutHandler, &ShortcutHandler::screenshotRequested, this, &AppController::takeScreenshot);
    connect(m_shortcutHandler, &ShortcutHandler::textRecognizeRequested, this, &AppController::takeTextRecognizeScreenshot);
    connect(m_shortcutHandler, &ShortcutHandler::formulaRecognizeRequested, this, &AppController::takeFormulaRecognizeScreenshot);
    connect(m_shortcutHandler, &ShortcutHandler::tableRecognizeRequested, this, &AppController::takeTableRecognizeScreenshot);

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
    connect(m_shortcutHandler, &ShortcutHandler::abortRequested, this, &AppController::abortRecognition);

    connect(m_trayManager, &TrayManager::screenshotRequested, this, &AppController::takeScreenshot);
    connect(m_trayManager, &TrayManager::textRecognizeRequested, this, &AppController::takeTextRecognizeScreenshot);
    connect(m_trayManager, &TrayManager::formulaRecognizeRequested, this, &AppController::takeFormulaRecognizeScreenshot);
    connect(m_trayManager, &TrayManager::tableRecognizeRequested, this, &AppController::takeTableRecognizeScreenshot);
    connect(m_trayManager, &TrayManager::showWindowRequested, this, &AppController::showWindow);
    connect(m_trayManager, &TrayManager::quitRequested, this, &AppController::quitApp);

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

    connect(m_mainWindow, &MainWindow::serviceSelected, this, &AppController::onServiceSelected);
    connect(m_mainWindow, &MainWindow::serviceToggleRequested, this, &AppController::toggleService);

    connect(m_mainWindow, &MainWindow::manualProcessRequested, this, &AppController::onManualProcessorTriggered);

    connect(m_mainWindow, &MainWindow::abortRequested, this, &AppController::abortRecognition);

    connect(m_screenshotManager, &ScreenshotManager::screenshotCaptured, this, &AppController::onScreenshotCaptured);
    connect(m_screenshotManager, &ScreenshotManager::screenshotFailed, this, &AppController::onScreenshotFailed);

    connect(m_recognitionManager, &RecognitionManager::streamDataReceived, m_mainWindow, &MainWindow::appendRecognitionResult);

    connect(m_recognitionManager, &RecognitionManager::busyStateChanged, this, [this](bool busy){
        if (busy) {
            emit recognitionResultReady(QString());
            m_mainWindow->setStreamingMode(true);

            if (m_settings->silentModeEnabled() && !m_mainWindow->isVisible()) {
                showSilentNotification(FloatingBall::Recognizing, "正在识别...");
            }
        } else {
            m_mainWindow->setStreamingMode(false);
        }
        emit busyStateChanged(busy);
    });

    connect(m_mainWindow, &MainWindow::loadHistoryRecordRequested, this, [this](int recordId){
        HistoryRecord record = HistoryManager::instance()->getRecordById(recordId);
        if (record.id != -1) {
            QImage img(record.cachedImagePath);
            if (!img.isNull()) {
                m_mainWindow->setImage(img);
                m_recognitionManager->setCurrentBase64(ImageProcessor::imageToBase64(img));
            }

            m_mainWindow->setRecognitionResult(record.resultText);
            m_mainWindow->setRecognizeType(record.recognitionType);

            QString prompt;
            switch(record.recognitionType) {
                case ContentType::Formula: prompt = m_settings->formulaPrompt(); break;
                case ContentType::Table: prompt = m_settings->tablePrompt(); break;
                default: prompt = m_settings->textPrompt();
            }
            m_mainWindow->setPrompt(prompt);

            showWindow();
        }
    });

    connect(m_recognitionManager, &RecognitionManager::recognitionFinished, this, [this](const QString& markdown){
        m_isRetryingAfterSwitch = false;
        m_serviceManager->resetIdleTimer();

        if (!markdown.isEmpty()) {
            emit recognitionResultReady(markdown);
        }

        QString finalText = m_mainWindow->currentMarkdownSource();
        m_mainWindow->setRecognizeType(m_lastRecognizeType);

        if (m_settings->saveHistoryEnabled()) {
            QImage currentImage = m_mainWindow->currentImage();

            if (!currentImage.isNull() && !finalText.isEmpty()) {
                HistoryManager::instance()->saveRecord(currentImage, finalText, m_lastRecognizeType);
            }
        }

        if (m_settings->autoCopyResult() && !finalText.isEmpty()) {
            m_copyProcessor->processAndCopy(finalText, m_lastRecognizeType);
        }

        if (m_settings->silentModeEnabled() && !m_mainWindow->isVisible()) {
            showSilentNotification(FloatingBall::Success, "识别完成");
        }
    });

    connect(m_recognitionManager, &RecognitionManager::recognitionFailed, this, [this](const QString& error){
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

    connect(m_recognitionManager, &RecognitionManager::recognitionAborted, this, [this](){
        m_isRetryingAfterSwitch = false;
        m_retryAfterServiceStart = false;
        m_pendingBase64.clear();
        m_pendingPrompt.clear();

        emit busyStateChanged(false);
        m_mainWindow->statusBar()->showMessage("识别已被用户中断", 3000);

        if (m_floatingBall) {
            m_floatingBall->setState(FloatingBall::Idle);
        }
    });

    connect(this, &AppController::imageChanged, m_mainWindow, &MainWindow::setImage);
    connect(this, &AppController::recognitionResultReady, m_mainWindow, &MainWindow::setRecognitionResult);
    connect(this, &AppController::recognitionFailed, m_mainWindow, &MainWindow::showError);

    connect(this, &AppController::busyStateChanged, m_mainWindow, &MainWindow::setBusy);

    connect(this, &AppController::requestAreaSelection, m_mainWindow, &MainWindow::startAreaSelection);

    connect(m_settings, &SettingsManager::shortcutsChanged, this, &AppController::onSettingsChanged);
    connect(m_settings, &SettingsManager::autoUseLastPromptChanged, m_recognitionManager, &RecognitionManager::setAutoUseLastPrompt);

    connect(m_settings, &SettingsManager::serviceProfilesChanged, this, [this](){
        m_mainWindow->updateServiceSelector(m_settings->serviceProfiles(), m_settings->currentServiceId());
        QString currentId = m_settings->currentServiceId();
        if (!currentId.isEmpty()) {
            applyServiceConfig(currentId);
        }
    });

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

    connect(m_settings, &SettingsManager::silentModeEnabledChanged, this, [this](bool enabled){
        if (!enabled && m_floatingBall) {
            m_floatingBall->setState(FloatingBall::Idle);
        }
    });

    connect(m_floatingBall, &FloatingBall::screenshotTriggered, this, &AppController::takeScreenshot);
    connect(m_floatingBall, &FloatingBall::showWindowTriggered, this, &AppController::onSilentNotificationClicked);
    connect(m_floatingBall, &FloatingBall::positionChanged, this, &AppController::onFloatingBallPositionChanged);

    connect(m_settings, &SettingsManager::floatingBallSizeChanged, this, &AppController::onFloatingBallSettingsChanged);
    connect(m_settings, &SettingsManager::floatingBallAutoHideTimeChanged, this, &AppController::onFloatingBallSettingsChanged);
    connect(m_settings, &SettingsManager::floatingBallAlwaysVisibleChanged, this, &AppController::onFloatingBallSettingsChanged);

    connect(m_copyProcessor, &CopyProcessor::error, m_mainWindow, &MainWindow::showError);
    connect(m_copyProcessor, &CopyProcessor::finished, this, [this](const QString& result){
        Q_UNUSED(result);
        m_mainWindow->statusBar()->showMessage("内容已处理并复制", 3000);
    });
}

inline void AppController::onServiceSelected(const QString& id)
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

inline void AppController::toggleService(const QString& id)
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

inline void AppController::applyServiceConfig(const QString& serviceId)
{
    QString textP, formulaP, tableP, url, apiKey, modelName, serviceName;

    if (serviceId.isEmpty()) {
        textP = m_settings->textPrompt();
        formulaP = m_settings->formulaPrompt();
        tableP = m_settings->tablePrompt();
        url = m_settings->serverUrl();
        apiKey = m_settings->globalApiKey();
        modelName = m_settings->globalModelName();
        serviceName = QString();
    } else {
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
            serviceName = p.name;
        }
    }

    m_recognitionManager->setServerUrl(url);
    m_recognitionManager->setApiKey(apiKey);
    m_recognitionManager->setModelName(modelName);

    m_currentServiceName = serviceName;

    m_mainWindow->setCurrentServiceName(serviceName);

    if (m_copyProcessor) {
        m_copyProcessor->setServiceName(serviceName);
    }

    qDebug() << "Applied service config. Name:" << serviceName
    << "URL:" << url << "Model:" << modelName;

    m_mainWindow->setCurrentPrompts(textP, formulaP, tableP);
    m_mainWindow->setPrompt(textP);
}

inline void AppController::takeScreenshot() {
    m_pendingPromptOverride = "";

    if (m_floatingBall) {
        m_floatingBall->hide();
    }

    m_screenshotManager->takeScreenshot();
}

inline void AppController::takeTextRecognizeScreenshot() {
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

inline void AppController::takeFormulaRecognizeScreenshot() {
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

inline void AppController::takeTableRecognizeScreenshot() {
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

inline void AppController::showWindow() {
    if (m_mainWindow) {
        m_mainWindow->show();
        m_mainWindow->raise();
        m_mainWindow->activateWindow();
    }
    if (m_floatingBall && m_floatingBall->currentState() != FloatingBall::Idle) {
        m_floatingBall->setState(FloatingBall::Idle);
    }
}

inline void AppController::quitApp() {
    SettingsManager::instance()->sync();
    QApplication::quit();
}

inline void AppController::onScreenshotCaptured(const QImage& image) {
    m_pendingFullScreenshot = image;
    emit requestAreaSelection(image);
}

inline void AppController::onScreenshotFailed(const QString& error) {
    if (m_floatingBall && m_settings->floatingBallAlwaysVisible()) {
        m_floatingBall->setState(FloatingBall::Idle);
    }

    if (m_mainWindow && m_mainWindow->isVisible()) {
        emit recognitionFailed(error);
    }
}

inline void AppController::onAreaSelected(const QRect& rect) {
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

inline void AppController::onSettingsChanged() {
    if (m_shortcutHandler) {
        m_shortcutHandler->applyShortcuts();
    }
}

inline void AppController::applySettings()
{
    if (m_recognitionManager && m_settings) {
        m_recognitionManager->setAutoUseLastPrompt(m_settings->autoUseLastPrompt());
    }
}

inline void AppController::onSingleInstanceMessageReceived(const QByteArray& message)
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

inline void AppController::onManualProcessorTriggered(ContentType type)
{
    QString currentText = m_mainWindow->currentMarkdownSource();
    if (currentText.isEmpty()) {
        m_mainWindow->statusBar()->showMessage("内容为空，无法处理。", 3000);
        return;
    }

    m_copyProcessor->manualProcess(currentText, type);
}

inline void AppController::showSilentNotification(FloatingBall::State state, const QString& message)
{
    if (!m_settings->silentModeEnabled()) return;

    QString notificationType = m_settings->silentModeNotificationType();

    if (notificationType == "floating_ball" && m_floatingBall) {
        m_floatingBall->setState(state, message);
    } else {
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

inline void AppController::onSilentNotificationClicked()
{
    showWindow();
}

inline void AppController::onFloatingBallPositionChanged(const QPoint& pos)
{
    m_settings->setFloatingBallPosX(pos.x());
    m_settings->setFloatingBallPosY(pos.y());
}

inline void AppController::onFloatingBallSettingsChanged()
{
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

inline void AppController::abortRecognition()
{
    if (m_recognitionManager) {
        m_recognitionManager->abortRecognition();
    }
}

#endif // APPCONTROLLER_H
