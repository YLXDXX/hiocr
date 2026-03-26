#include "appcontroller.h"
#include "mainwindow.h"
#include "screenshotmanager.h"
#include "recognitionmanager.h"
#include "settingsmanager.h"
#include "traymanager.h"
#include "shortcuthandler.h"
#include "imageprocessor.h"
#include "settingsdialog.h"

#include <QApplication>
#include <QTimer>
#include <QDebug>
#include <QLocalServer>
#include <QLocalSocket>
#include <QJsonDocument>
#include <QJsonObject>


// 单实例服务名称
static const QString SINGLE_INSTANCE_KEY = "hiocr_single_instance_socket";

AppController::AppController(QObject *parent) : QObject(parent)
{
}

AppController::~AppController()
{
    if (m_mainWindow) {
        delete m_mainWindow;
        m_mainWindow = nullptr;
    }
    // 清理单实例服务器
    if (m_localServer) {
        m_localServer->close();
        delete m_localServer;
    }
}

void AppController::initialize()
{
    setupManagers();
    setupSingleInstance(); // 【新增】设置单实例监听
    setupConnections();
    applySettings();
}

// 【新增】处理外部传入的参数（来自第二次启动或命令行）
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

    // 1. 显示主窗口（必须）
    showWindow();

    // 2. 如果有识别结果，进入“静态展示模式”
    if (!resultText.isEmpty()) {
        if (!img.isNull()) {
            emit imageChanged(img);
            // 设置当前图片数据，以便用户后续可能手动点击“识别”
            QString base64 = ImageProcessor::imageToBase64(img);
            m_recognitionManager->setCurrentBase64(base64);
        }
        // 设置结果并显示
        emit recognitionResultReady(resultText);
        // 【关键】不触发自动识别，不启动服务，直接结束
        return;
    }

    // 3. 如果只有图片路径，执行现有逻辑（自动识别或仅加载）
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


// 【新增】设置单实例通信服务器
void AppController::setupSingleInstance()
{
    m_localServer = new QLocalServer(this);
    connect(m_localServer, &QLocalServer::newConnection, this, &AppController::onNewInstanceConnected);

    // 尝试监听，如果失败说明已有实例（但在 main.cpp 中已确保此时我们是主实例）
    if (!m_localServer->listen(SINGLE_INSTANCE_KEY)) {
        // 如果 key 存在残留文件，尝试移除后重试
        QLocalServer::removeServer(SINGLE_INSTANCE_KEY);
        m_localServer->listen(SINGLE_INSTANCE_KEY);
    }
}

// 【新增】处理新实例的连接
void AppController::onNewInstanceConnected()
{
    QLocalSocket* socket = m_localServer->nextPendingConnection();
    if (!socket) return;

    // 阻塞等待数据读取
    if (socket->waitForReadyRead(500)) {
        QByteArray data = socket->readAll();
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(data, &err);

        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject obj = doc.object();
            QString image = obj["image"].toString();
            QString result = obj["result"].toString();
            handleCommandLineArguments(image, result);
        }
    }
    socket->deleteLater();
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
    connect(m_mainWindow, &MainWindow::recognizeRequested, this, [this](const QString& prompt, const QString& base64Img){
        m_pendingPrompt = prompt;
        m_pendingBase64 = base64Img;
        m_serviceManager->ensureRunning();
    });
    connect(m_mainWindow, &MainWindow::imagePasted, this, [this](const QImage& img){
        m_mainWindow->setImage(img);
        m_recognitionManager->onImageChanged(ImageProcessor::imageToBase64(img));
    });
    connect(m_mainWindow, &MainWindow::areaSelected, this, &AppController::onAreaSelected);
    connect(m_mainWindow, &MainWindow::screenshotRequested, this, &AppController::takeScreenshot);
    connect(m_mainWindow, &MainWindow::stopServiceRequested, m_serviceManager, &ServiceManager::stopService);
    connect(m_mainWindow, &MainWindow::settingsTriggered, this, [this](){
        SettingsDialog dialog(m_mainWindow);
        dialog.exec();
    });

    // ScreenshotManager -> Controller
    connect(m_screenshotManager, &ScreenshotManager::screenshotCaptured, this, &AppController::onScreenshotCaptured);
    connect(m_screenshotManager, &ScreenshotManager::screenshotFailed, this, &AppController::onScreenshotFailed);

    // RecognitionManager -> Controller
    connect(m_recognitionManager, &RecognitionManager::recognitionFinished, this, [this](const QString& markdown){
        m_serviceManager->resetIdleTimer();
        onRecognitionFinished(markdown);
    });

    connect(m_recognitionManager, &RecognitionManager::recognitionFailed, this, [this](const QString& error){
        if (error.contains("Connection refused") || error.contains("连接被拒绝")) {
            if (SettingsManager::instance()->autoStartService()) {
                qDebug() << "Service connection failed, attempting restart...";
                m_retryAfterServiceStart = true;
                m_serviceManager->ensureRunning();
                return;
            }
        }
        emit recognitionFailed(error);
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

    // ServiceManager -> Controller
    connect(m_serviceManager, &ServiceManager::serviceReady, this, [this](){
        if (!m_pendingBase64.isEmpty() && !m_pendingPrompt.isEmpty()) {
            m_recognitionManager->recognize(m_pendingPrompt, m_pendingBase64);
            m_pendingBase64.clear();
            m_pendingPrompt.clear();
            m_retryAfterServiceStart = false;
        }
        else if (m_retryAfterServiceStart) {
            QString b64 = m_recognitionManager->currentBase64();
            QString prompt = m_recognitionManager->lastPrompt();
            if (!b64.isEmpty() && !prompt.isEmpty()) {
                qDebug() << "Retrying recognition after service start...";
                m_recognitionManager->recognize(prompt, b64);
            }
            m_retryAfterServiceStart = false;
        }
    });

    connect(m_serviceManager, &ServiceManager::errorOccurred, this, [this](const QString& error){
        emit recognitionFailed("服务启动失败: " + error);
    });

    connect(m_serviceManager, &ServiceManager::runningStateChanged, m_mainWindow, &MainWindow::updateStopServiceAction);
}

// --- 动作实现 ---

void AppController::takeScreenshot() {
    m_pendingPromptOverride = "";
    m_screenshotManager->takeScreenshot();
}

void AppController::takeTextRecognizeScreenshot() {
    QString prompt = SettingsManager::instance()->textPrompt();
    m_pendingPromptOverride = prompt;
    m_mainWindow->setPrompt(prompt);
    m_screenshotManager->takeScreenshot();
}

void AppController::takeFormulaRecognizeScreenshot() {
    QString prompt = SettingsManager::instance()->formulaPrompt();
    m_pendingPromptOverride = prompt;
    m_mainWindow->setPrompt(prompt);
    m_screenshotManager->takeScreenshot();
}

void AppController::takeTableRecognizeScreenshot() {
    QString prompt = SettingsManager::instance()->tablePrompt();
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

// --- 内部槽 ---

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
            QApplication::clipboard()->setText(markdown);
            // 移除托盘通知，只静默复制
            // if (m_trayManager) {
            //     m_trayManager->showMessage("识别完成", "结果已自动复制到剪贴板");
            // }
        }
    }
}
