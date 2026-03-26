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

AppController::AppController(QObject *parent) : QObject(parent)
{
}

// 【修改】实现析构函数，显式删除 MainWindow
AppController::~AppController()
{
    if (m_mainWindow) {
        delete m_mainWindow;
        m_mainWindow = nullptr;
    }
    // 其他的 Manager（如 ScreenshotManager, TrayManager）由于设置了 this 为 parent，
    // 会由 Qt 的对象树机制自动销毁，无需手动删除。
}

void AppController::initialize()
{
    setupManagers();
    setupConnections();
    applySettings();
}

MainWindow* AppController::mainWindow() const
{
    return m_mainWindow;
}

void AppController::setupManagers()
{
    // 1. 设置管理器
    m_settings = SettingsManager::instance();

    // 2. 核心业务管理器
    m_screenshotManager = new ScreenshotManager(this);
    m_recognitionManager = new RecognitionManager(this);

    // 3. UI 层
    m_mainWindow = new MainWindow();
    m_trayManager = new TrayManager(this);

    // 4. 快捷键处理器
    m_shortcutHandler = new ShortcutHandler(this);

    m_serviceManager = new ServiceManager(this); // 新增
}

void AppController::setupConnections()
{
    // --- 快捷键 -> 动作 ---
    connect(m_shortcutHandler, &ShortcutHandler::screenshotRequested, this, &AppController::takeScreenshot);
    connect(m_shortcutHandler, &ShortcutHandler::textRecognizeRequested, this, &AppController::takeTextRecognizeScreenshot);
    connect(m_shortcutHandler, &ShortcutHandler::formulaRecognizeRequested, this, &AppController::takeFormulaRecognizeScreenshot);
    connect(m_shortcutHandler, &ShortcutHandler::tableRecognizeRequested, this, &AppController::takeTableRecognizeScreenshot);

    // --- 托盘 -> 动作 ---
    connect(m_trayManager, &TrayManager::screenshotRequested, this, &AppController::takeScreenshot);
    connect(m_trayManager, &TrayManager::textRecognizeRequested, this, &AppController::takeTextRecognizeScreenshot);
    connect(m_trayManager, &TrayManager::formulaRecognizeRequested, this, &AppController::takeFormulaRecognizeScreenshot);
    connect(m_trayManager, &TrayManager::tableRecognizeRequested, this, &AppController::takeTableRecognizeScreenshot);
    connect(m_trayManager, &TrayManager::showWindowRequested, this, &AppController::showWindow);
    connect(m_trayManager, &TrayManager::quitRequested, this, &AppController::quitApp);

    // --- MainWindow UI -> Controller ---

    // 连接识别请求
    connect(m_mainWindow, &MainWindow::recognizeRequested, this, [this](const QString& prompt, const QString& base64Img){
        // 1. 暂存请求参数
        m_pendingPrompt = prompt;
        m_pendingBase64 = base64Img;

        // 2. 尝试确保服务运行
        m_serviceManager->ensureRunning();
    });

    connect(m_mainWindow, &MainWindow::imagePasted, this, [this](const QImage& img){
        m_mainWindow->setImage(img);
        m_recognitionManager->onImageChanged(ImageProcessor::imageToBase64(img));
    });
    connect(m_mainWindow, &MainWindow::areaSelected, this, &AppController::onAreaSelected);

    // 连接菜单栏的截图请求
    connect(m_mainWindow, &MainWindow::screenshotRequested, this, &AppController::takeScreenshot);

    // 连接停止服务请求
    connect(m_mainWindow, &MainWindow::stopServiceRequested, m_serviceManager, &ServiceManager::stopService);

    // 连接设置窗口请求
    connect(m_mainWindow, &MainWindow::settingsTriggered, this, [this](){
        SettingsDialog dialog(m_mainWindow);
        dialog.exec(); // 模态运行
    });

    // --- ScreenshotManager -> Controller ---
    connect(m_screenshotManager, &ScreenshotManager::screenshotCaptured, this, &AppController::onScreenshotCaptured);
    connect(m_screenshotManager, &ScreenshotManager::screenshotFailed, this, &AppController::onScreenshotFailed);

    // --- RecognitionManager -> Controller ---
    // 【修改】合并识别完成的处理逻辑：重置空闲计时器 + 处理结果
    connect(m_recognitionManager, &RecognitionManager::recognitionFinished, this, [this](const QString& markdown){
        // 重置服务空闲计时器
        m_serviceManager->resetIdleTimer();
        // 处理结果（更新UI、自动复制等）
        onRecognitionFinished(markdown);
    });

    // 【修改】处理识别失败逻辑：检测连接拒绝并触发重试
    connect(m_recognitionManager, &RecognitionManager::recognitionFailed, this, [this](const QString& error){
        // 检测错误类型：如果是连接被拒绝，且配置了自动启动，则尝试重启
        if (error.contains("Connection refused") || error.contains("连接被拒绝")) {
            if (SettingsManager::instance()->autoStartService()) {
                qDebug() << "Service connection failed, attempting restart...";

                // 【关键修复】设置重试标志，服务启动成功后将自动重试
                m_retryAfterServiceStart = true;

                // 尝试重新启动服务
                m_serviceManager->ensureRunning();
                return; // 不直接显示错误，等待服务启动后的重试
            }
        }
        // 其他错误直接显示
        emit recognitionFailed(error);
    });

    connect(m_recognitionManager, &RecognitionManager::busyStateChanged, this, &AppController::busyStateChanged);

    // --- Controller -> MainWindow (更新 UI) ---
    connect(this, &AppController::imageChanged, m_mainWindow, &MainWindow::setImage);
    connect(this, &AppController::recognitionResultReady, m_mainWindow, &MainWindow::setRecognitionResult);
    connect(this, &AppController::recognitionFailed, m_mainWindow, &MainWindow::showError);
    connect(this, &AppController::busyStateChanged, m_mainWindow, &MainWindow::setBusy);
    connect(this, &AppController::requestAreaSelection, m_mainWindow, &MainWindow::startAreaSelection);

    // --- 设置变更 ---
    connect(m_settings, &SettingsManager::shortcutsChanged, this, &AppController::onSettingsChanged);
    connect(m_settings, &SettingsManager::autoUseLastPromptChanged, m_recognitionManager, &RecognitionManager::setAutoUseLastPrompt);

    // --- ServiceManager -> Controller ---
    // 【修改】服务就绪后的处理逻辑：处理新请求 或 处理重试请求
    connect(m_serviceManager, &ServiceManager::serviceReady, this, [this](){
        // 1. 优先处理手动触发的新请求
        if (!m_pendingBase64.isEmpty() && !m_pendingPrompt.isEmpty()) {
            m_recognitionManager->recognize(m_pendingPrompt, m_pendingBase64);
            m_pendingBase64.clear();
            m_pendingPrompt.clear();
            m_retryAfterServiceStart = false; // 清除重试标志，因为已经有了新请求
        }
        // 2. 处理连接失败后的自动重试请求
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

    // 服务错误处理
    connect(m_serviceManager, &ServiceManager::errorOccurred, this, [this](const QString& error){
        emit recognitionFailed("服务启动失败: " + error);
    });

    // 【新增】连接服务状态变化到 MainWindow 的菜单更新
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
    // 【新增】在退出前显式保存配置
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
        return; // 用户取消
    }

    QImage cropped = m_pendingFullScreenshot.copy(rect);
    m_pendingFullScreenshot = QImage();

    // 显示图片
    emit imageChanged(cropped);

    // 【修改】检查设置：是否自动识别
    if (m_settings->autoRecognizeOnScreenshot()) {
        QString base64 = ImageProcessor::imageToBase64(cropped);
        if (!m_pendingPromptOverride.isEmpty()) {
            m_recognitionManager->setTempPromptOverride(m_pendingPromptOverride);
            m_pendingPromptOverride.clear();
        }
        // 触发识别流程
        m_recognitionManager->onImageChanged(base64);
    } else {
        // 如果不自动识别，只显示窗口，不发起网络请求
        // 清空上一次的结果，避免混淆
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
    // 可以考虑在这里发信号通知 MainWindow 显示提示
}

void AppController::applySettings()
{
    // 应用初始设置
    if (m_recognitionManager && m_settings) {
        m_recognitionManager->setAutoUseLastPrompt(m_settings->autoUseLastPrompt());
    }
    // 快捷键在 ShortcutHandler 构造时已自动应用，这里无需重复调用
}

void AppController::onRecognitionFinished(const QString& markdown)
{
    // 1. 更新 UI
    emit recognitionResultReady(markdown);

    // 2. 【新增】处理自动复制
    if (m_settings->autoCopyResult()) {
        if (!markdown.isEmpty()) {
            QApplication::clipboard()->setText(markdown);
            // 可选：在托盘或状态栏提示已复制
            if (m_trayManager) {
                m_trayManager->showMessage("识别完成", "结果已自动复制到剪贴板");
            }
        }
    }
}
