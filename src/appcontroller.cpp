#include "appcontroller.h"
#include "mainwindow.h"
#include "screenshotmanager.h"
#include "recognitionmanager.h"
#include "settingsmanager.h"
#include "traymanager.h"
#include "shortcuthandler.h"
#include "imageprocessor.h"
#include "settingsdialog.h" // 新增
#include "constants.h"      // 新增

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
        m_recognitionManager->recognize(prompt, base64Img);
    });

    connect(m_mainWindow, &MainWindow::imagePasted, this, [this](const QImage& img){
        m_mainWindow->setImage(img);
        m_recognitionManager->onImageChanged(ImageProcessor::imageToBase64(img));
    });
    connect(m_mainWindow, &MainWindow::areaSelected, this, &AppController::onAreaSelected);

    // 连接菜单栏的截图请求
    connect(m_mainWindow, &MainWindow::screenshotRequested, this, &AppController::takeScreenshot);

    // --- ScreenshotManager -> Controller ---
    connect(m_screenshotManager, &ScreenshotManager::screenshotCaptured, this, &AppController::onScreenshotCaptured);
    connect(m_screenshotManager, &ScreenshotManager::screenshotFailed, this, &AppController::onScreenshotFailed);

    // --- RecognitionManager -> Controller ---
    // 【修改】连接到新的槽函数 onRecognitionFinished，以便处理自动复制逻辑
    connect(m_recognitionManager, &RecognitionManager::recognitionFinished, this, &AppController::onRecognitionFinished);
    connect(m_recognitionManager, &RecognitionManager::recognitionFailed, this, &AppController::recognitionFailed);
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

    connect(m_mainWindow, &MainWindow::settingsTriggered, this, [](){
        qDebug() << "Settings menu interaction";
    });

    // 连接设置窗口请求
    connect(m_mainWindow, &MainWindow::settingsTriggered, this, [this](){
        SettingsDialog dialog(m_mainWindow);
        dialog.exec(); // 模态运行
    });
}

// --- 动作实现 ---

void AppController::takeScreenshot() {
    m_pendingPromptOverride = "";
    m_screenshotManager->takeScreenshot();
}

void AppController::takeTextRecognizeScreenshot() {
    QString prompt = Constants::PROMPT_TEXT;
    m_pendingPromptOverride = prompt;
    m_mainWindow->setPrompt(prompt);
    m_screenshotManager->takeScreenshot();
}

void AppController::takeFormulaRecognizeScreenshot() {
    QString prompt = Constants::PROMPT_FORMULA;
    m_pendingPromptOverride = prompt;
    m_mainWindow->setPrompt(prompt);
    m_screenshotManager->takeScreenshot();
}

void AppController::takeTableRecognizeScreenshot() {
    QString prompt = Constants::PROMPT_TABLE;
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
