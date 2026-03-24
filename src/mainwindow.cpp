#include "mainwindow.h"
#include "imageviewwidget.h"
#include "markdownrenderer.h"
#include "markdownsourceeditor.h"
#include "promptbar.h"
#include "screenshotmanager.h"
#include "settingsmanager.h"
#include "traymanager.h"
#include "recognitionmanager.h"
#include "globalshortcutmanager.h"

#ifdef HAVE_KF6
#include "kdeglobalshortcut.h"
#endif

#include <QSplitter>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QTimer>

MainWindow::MainWindow(QWidget* parent)
: QMainWindow(parent)
{
    initManagers(); // 先初始化管理器
    setupUi();
    setupConnections();
    setupMenuBar();
    applyGlobalShortcuts();
}

MainWindow::~MainWindow() {}

void MainWindow::initManagers()
{
    // 1. 设置管理器（单例）
    m_settings = SettingsManager::instance();

    // 2. 截图管理器
    m_screenshotManager = new ScreenshotManager(this);

    // 3. 托盘管理器
    m_trayManager = new TrayManager(this);

    // 4. 识别管理器
    m_recognitionManager = new RecognitionManager(this);
    m_recognitionManager->setAutoUseLastPrompt(m_settings->autoUseLastPrompt());
}

void MainWindow::setupUi()
{
    setWindowTitle("hiocr - 文字识别");
    resize(1200, 800);

    QSplitter* mainSplitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(mainSplitter);

    // 左侧：图片 + 渲染结果
    QSplitter* leftSplitter = new QSplitter(Qt::Vertical);
    mainSplitter->addWidget(leftSplitter);

    m_imageView = new ImageViewWidget(this);
    leftSplitter->addWidget(m_imageView);

    m_markdownRenderer = new MarkdownRenderer(this);
    leftSplitter->addWidget(m_markdownRenderer);
    leftSplitter->setSizes({400, 400});

    // 右侧：提示词 + 源码编辑
    QSplitter* rightSplitter = new QSplitter(Qt::Vertical);
    mainSplitter->addWidget(rightSplitter);

    m_promptBar = new PromptBar(this);
    rightSplitter->addWidget(m_promptBar);

    m_markdownSource = new MarkdownSourceEditor(this);
    rightSplitter->addWidget(m_markdownSource);
    rightSplitter->setStretchFactor(1, 1);

    mainSplitter->setSizes({700, 500});

    // 粘贴快捷键
    m_pasteShortcut = new QShortcut(QKeySequence::Paste, this);
    m_pasteShortcut->setContext(Qt::ApplicationShortcut);
}

void MainWindow::setupConnections()
{
    // --- UI 信号 ---
    // 粘贴
    connect(m_pasteShortcut, &QShortcut::activated, this, &MainWindow::onPasteImage);
    // Markdown 源码编辑变化 -> 更新渲染
    connect(m_markdownSource, &QPlainTextEdit::textChanged, this, &MainWindow::onMarkdownSourceChanged);
    // 图片变化 -> 通知识别管理器（触发防抖）
    connect(m_imageView, &ImageViewWidget::imageChanged, this, [this]() {
        m_recognitionManager->onImageChanged(m_imageView->currentBase64());
    });

    // PromptBar 信号
    connect(m_promptBar, &PromptBar::recognizeRequested, this, [this]() {
        m_recognitionManager->recognize(m_promptBar->prompt(), m_imageView->currentBase64());
    });
    connect(m_promptBar, &PromptBar::autoRecognizeRequested, this, [this](const QString& prompt) {
        m_recognitionManager->recognize(prompt, m_imageView->currentBase64());
    });

    // --- Manager 信号 ---
    // 截图成功
    connect(m_screenshotManager, &ScreenshotManager::screenshotCaptured, this, [this](const QImage& image) {
        m_imageView->setImage(image);
        // 延迟显示窗口（兼容 Wayland）
        QTimer::singleShot(0, this, [this]() {
            if (!isVisible()) show();
            raise();
            activateWindow();
        });
    });
    connect(m_screenshotManager, &ScreenshotManager::screenshotFailed, this, [this](const QString& err) {
        if (isVisible()) QMessageBox::warning(this, "截图失败", err);
    });

        // 识别结果
        connect(m_recognitionManager, &RecognitionManager::recognitionFinished, this, &MainWindow::onRecognitionFinished);
        connect(m_recognitionManager, &RecognitionManager::recognitionFailed, this, &MainWindow::onRecognitionFailed);
        connect(m_recognitionManager, &RecognitionManager::busyStateChanged, this, &MainWindow::onBusyChanged);

        // 托盘信号 -> 动作
        connect(m_trayManager, &TrayManager::quitRequested, qApp, &QApplication::quit);
        connect(m_trayManager, &TrayManager::showWindowRequested, this, &MainWindow::onActionShowWindow);
        connect(m_trayManager, &TrayManager::screenshotRequested, this, &MainWindow::onActionScreenshot);
        connect(m_trayManager, &TrayManager::textRecognizeRequested, this, &MainWindow::onActionTextRecognize);
        connect(m_trayManager, &TrayManager::formulaRecognizeRequested, this, &MainWindow::onActionFormulaRecognize);
        connect(m_trayManager, &TrayManager::tableRecognizeRequested, this, &MainWindow::onActionTableRecognize);

        // 设置变更
        connect(m_settings, &SettingsManager::shortcutsChanged, this, &MainWindow::onShortcutSettingChanged);
}

// --- 菜单栏逻辑 ---
void MainWindow::setupMenuBar()
{
    QMenuBar* menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    QMenu* fileMenu = menuBar->addMenu("文件");
    fileMenu->addAction("打开", this, [this]() {
        QString file = QFileDialog::getOpenFileName(this, "选择图片", "", "Images (*.png *.jpg *.bmp)");
        if (!file.isEmpty()) m_imageView->setImage(file);
    });
    fileMenu->addAction("保存", this, [this]() {
        QString content = m_markdownSource->toPlainText();
        if (content.isEmpty()) return;
        QString fileName = QFileDialog::getSaveFileName(this, "保存Markdown", "", "*.md");
        if (fileName.isEmpty()) return;
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream(&file) << content;
            file.close();
        }
    });
    fileMenu->addAction("截图", this, &MainWindow::onActionScreenshot);
    fileMenu->addSeparator();
    fileMenu->addAction("退出", qApp, &QApplication::quit);

    QMenu* settingsMenu = menuBar->addMenu("设置");

    // 设置服务器地址
    settingsMenu->addAction("设置服务器地址", this, [this]() {
        bool ok;
        QString url = QInputDialog::getText(this, "服务器地址", "URL:", QLineEdit::Normal, m_settings->serverUrl(), &ok);
        if (ok) m_settings->setServerUrl(url);
    });

    // 使用 Lambda 辅助添加快捷键设置菜单
    // 参数：菜单名称，获取当前快捷键的 lambda，设置快捷键的 lambda
    auto addShortcutAction = [this, settingsMenu](const QString& name,
                                                  std::function<QString()> getter,
                                                  std::function<void(const QString&)> setter) {
        settingsMenu->addAction(name, this, [this, name, getter, setter]() {
            bool ok;
            // 使用 getter 获取当前快捷键
            QString current = getter();
            QString key = QInputDialog::getText(this, "设置快捷键", name + " (如 Ctrl+Shift+S):", QLineEdit::Normal, current, &ok);
            if (ok && !key.isEmpty()) {
                setter(key);
            }
        });
    };

    // 修正：使用 lambda 包装成员函数调用
    addShortcutAction("设置截图快捷键",
                      [this](){ return m_settings->screenshotShortcut(); },
                      [this](const QString& k){ m_settings->setScreenshotShortcut(k); });

    addShortcutAction("设置文字识别快捷键",
                      [this](){ return m_settings->textRecognizeShortcut(); },
                      [this](const QString& k){ m_settings->setTextRecognizeShortcut(k); });

    addShortcutAction("设置公式识别快捷键",
                      [this](){ return m_settings->formulaRecognizeShortcut(); },
                      [this](const QString& k){ m_settings->setFormulaRecognizeShortcut(k); });

    addShortcutAction("设置表格识别快捷键",
                      [this](){ return m_settings->tableRecognizeShortcut(); },
                      [this](const QString& k){ m_settings->setTableRecognizeShortcut(k); });

    QAction* autoUseAction = settingsMenu->addAction("自动使用上次识别类型");
    autoUseAction->setCheckable(true);
    autoUseAction->setChecked(m_settings->autoUseLastPrompt());
    connect(autoUseAction, &QAction::toggled, m_settings, &SettingsManager::setAutoUseLastPrompt);
    connect(m_settings, &SettingsManager::autoUseLastPromptChanged, autoUseAction, &QAction::setChecked);
}

// --- 动作处理 ---

void MainWindow::onActionScreenshot() {
    m_recognitionManager->setTempPromptOverride(""); // 清空临时提示词，让逻辑走默认
    m_screenshotManager->takeScreenshot();
}

void MainWindow::onActionTextRecognize() {
    m_recognitionManager->setTempPromptOverride("文字识别:");
    m_screenshotManager->takeScreenshot();
}

void MainWindow::onActionFormulaRecognize() {
    m_recognitionManager->setTempPromptOverride("公式识别:");
    m_screenshotManager->takeScreenshot();
}

void MainWindow::onActionTableRecognize() {
    m_recognitionManager->setTempPromptOverride("表格识别:");
    m_screenshotManager->takeScreenshot();
}

void MainWindow::onActionShowWindow() {
    show();
    raise();
    activateWindow();
}

// --- 事件处理 ---

void MainWindow::closeEvent(QCloseEvent *event)
{
    event->ignore();
    hide();
    static bool firstTime = true;
    if (firstTime) {
        m_trayManager->showMessage("hiocr", "程序已最小化到系统托盘");
        firstTime = false;
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) close();
    else QMainWindow::keyPressEvent(event);
}

// --- 业务逻辑槽 ---

void MainWindow::onRecognitionFinished(const QString& result) {
    m_markdownSource->setPlainText(result);
    m_markdownRenderer->setMarkdown(result);
}

void MainWindow::onRecognitionFailed(const QString& error) {
    QMessageBox::critical(this, "识别失败", error);
}

void MainWindow::onBusyChanged(bool busy) {
    m_promptBar->setButtonsBusy(busy);
}

void MainWindow::onMarkdownSourceChanged() {
    m_markdownRenderer->setMarkdown(m_markdownSource->toPlainText());
}

void MainWindow::onPasteImage() {
    // 检查焦点是否在输入框，如果是则执行默认粘贴文本
    QWidget* focus = QApplication::focusWidget();
    if (qobject_cast<QLineEdit*>(focus) || qobject_cast<QPlainTextEdit*>(focus) || qobject_cast<QTextEdit*>(focus)) {
        if (auto* edit = qobject_cast<QPlainTextEdit*>(focus)) edit->paste();
        else if (auto* lineEdit = qobject_cast<QLineEdit*>(focus)) lineEdit->paste();
        return;
    }

    QClipboard* clipboard = QApplication::clipboard();
    const QMimeData* mimeData = clipboard->mimeData();
    if (mimeData->hasImage()) {
        QImage image = clipboard->image();
        if (!image.isNull()) m_imageView->setImage(image);
    }
}

void MainWindow::loadImageFromFile(const QString& path) {
    m_imageView->setImage(path);
}

// --- 快捷键管理 ---

void MainWindow::applyGlobalShortcuts()
{
    // 清理旧的本地快捷键
    delete m_shortcutScreenshot; m_shortcutScreenshot = nullptr;
    delete m_shortcutText; m_shortcutText = nullptr;
    delete m_shortcutFormula; m_shortcutFormula = nullptr;
    delete m_shortcutTable; m_shortcutTable = nullptr;

    // 注册本地快捷键（适用于 X11 或应用内）
    auto createLocal = [this](QString key, QShortcut*& ptr, auto slot) {
        if (key.isEmpty()) return;
        ptr = new QShortcut(QKeySequence(key), this);
        ptr->setContext(Qt::ApplicationShortcut);
        connect(ptr, &QShortcut::activated, this, slot);
    };

    createLocal(m_settings->screenshotShortcut(), m_shortcutScreenshot, &MainWindow::onActionScreenshot);
    createLocal(m_settings->textRecognizeShortcut(), m_shortcutText, &MainWindow::onActionTextRecognize);
    createLocal(m_settings->formulaRecognizeShortcut(), m_shortcutFormula, &MainWindow::onActionFormulaRecognize);
    createLocal(m_settings->tableRecognizeShortcut(), m_shortcutTable, &MainWindow::onActionTableRecognize);

    // 注册全局快捷键（Wayland Portal / KDE）
    if (QGuiApplication::platformName() == "wayland") {
        bool isKde = qgetenv("XDG_CURRENT_DESKTOP").toLower().contains("kde");
        #ifdef HAVE_KF6
        if (isKde) {
            KdeGlobalShortcut* ks = KdeGlobalShortcut::instance();
            disconnect(ks, nullptr, this, nullptr);
            connect(ks, &KdeGlobalShortcut::activated, this, [this](const QString& id) {
                if (id == "screenshot") onActionScreenshot();
                else if (id == "text_recognize") onActionTextRecognize();
                else if (id == "formula_recognize") onActionFormulaRecognize();
                else if (id == "table_recognize") onActionTableRecognize();
            });
                ks->registerShortcut("screenshot", "Screenshot", m_settings->screenshotShortcut());
                ks->registerShortcut("text_recognize", "Text", m_settings->textRecognizeShortcut());
                ks->registerShortcut("formula_recognize", "Formula", m_settings->formulaRecognizeShortcut());
                ks->registerShortcut("table_recognize", "Table", m_settings->tableRecognizeShortcut());
                ks->startListening();
                return;
        }
        #endif

        // Portal 方式
        GlobalShortcutManager* gsm = GlobalShortcutManager::instance();
        disconnect(gsm, nullptr, this, nullptr);
        connect(gsm, &GlobalShortcutManager::activated, this, [this](const QString& id) {
            if (id == "screenshot") onActionScreenshot();
            else if (id == "text_recognize") onActionTextRecognize();
            else if (id == "formula_recognize") onActionFormulaRecognize();
            else if (id == "table_recognize") onActionTableRecognize();
        });
            gsm->registerShortcut("screenshot", "Screenshot", m_settings->screenshotShortcut());
            gsm->registerShortcut("text_recognize", "Text", m_settings->textRecognizeShortcut());
            gsm->registerShortcut("formula_recognize", "Formula", m_settings->formulaRecognizeShortcut());
            gsm->registerShortcut("table_recognize", "Table", m_settings->tableRecognizeShortcut());
            gsm->startListening();
    }
}

void MainWindow::onShortcutSettingChanged() {
    applyGlobalShortcuts();
    QMessageBox::information(this, "提示", "快捷键已更新，部分环境可能需要重启生效。");
}
