#include "mainwindow.h"
#include "imageviewwidget.h"
#include "markdownrenderer.h"
#include "markdownsourceeditor.h"
#include "promptbar.h"
#include "imageprocessor.h"
#include "screenshotareaselector.h"
#include "settingsmanager.h" // 引入设置管理器
#include "markdowncopybar.h" // 引入新组件

#include <QSplitter>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QInputDialog>
#include <QLineEdit>
#include <QKeyEvent>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setupUi();
    setupMenuBar(); // 构建菜单
    setupConnections();
}

MainWindow::~MainWindow() {}

void MainWindow::setupUi()
{
    setWindowTitle("hiocr - 文字识别");
    resize(1200, 800);

    QSplitter* mainSplitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(mainSplitter);

    // 左侧
    QSplitter* leftSplitter = new QSplitter(Qt::Vertical);
    mainSplitter->addWidget(leftSplitter);

    m_imageView = new ImageViewWidget(this);
    leftSplitter->addWidget(m_imageView);

    m_markdownRenderer = new MarkdownRenderer(this);
    leftSplitter->addWidget(m_markdownRenderer);
    leftSplitter->setSizes({400, 400});

    // 右侧布局调整
    QSplitter* rightSplitter = new QSplitter(Qt::Vertical);
    mainSplitter->addWidget(rightSplitter);

    m_promptBar = new PromptBar(this);
    rightSplitter->addWidget(m_promptBar);

    // 创建 Markdown 容器
    QWidget* markdownContainer = new QWidget(this);
    QVBoxLayout* mdLayout = new QVBoxLayout(markdownContainer);
    mdLayout->setContentsMargins(0, 0, 0, 0);
    mdLayout->setSpacing(2);

    m_markdownSource = new MarkdownSourceEditor(this);

    // 创建复制栏
    m_copyBar = new MarkdownCopyBar(this);

    // 【关键修改】将编辑器指针传递给复制栏
    m_copyBar->setSourceEditor(m_markdownSource);

    mdLayout->addWidget(m_markdownSource);
    mdLayout->addWidget(m_copyBar);

    rightSplitter->addWidget(markdownContainer);
    rightSplitter->setStretchFactor(1, 1);

    mainSplitter->setSizes({700, 500});

    m_pasteShortcut = new QShortcut(QKeySequence::Paste, this);
    m_pasteShortcut->setContext(Qt::ApplicationShortcut);
}


void MainWindow::setupMenuBar()
{
    QMenuBar* menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    // --- 文件菜单 ---
    QMenu* fileMenu = menuBar->addMenu("文件");
    fileMenu->addAction("打开图片", this, [this]() {
        QString file = QFileDialog::getOpenFileName(this, "选择图片", "", "Images (*.png *.jpg *.bmp)");
        if (!file.isEmpty()) emit imagePasted(QImage(file));
    });
        fileMenu->addAction("保存结果", this, [this]() {
            QString content = m_markdownSource->toPlainText();
            if (content.isEmpty()) return;
            QString fileName = QFileDialog::getSaveFileName(this, "保存Markdown", "", "*.md");
            if (!fileName.isEmpty()) {
                QFile file(fileName);
                if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    QTextStream(&file) << content;
                }
            }
        });
        fileMenu->addAction("截图", this, [this](){
            emit screenshotRequested();
        });
        fileMenu->addSeparator();
        fileMenu->addAction("退出", qApp, &QApplication::quit);

        // --- 设置菜单 ---
        QMenu* settingsMenu = menuBar->addMenu("设置");
        // 【修改】不再直接弹出输入框，而是发射信号通知 AppController 打开设置窗口
        settingsMenu->addAction("首选项", this, [this](){
            emit settingsTriggered();
        });
}

void MainWindow::setupConnections()
{
    // 内部 UI 连接
    connect(m_markdownSource, &QPlainTextEdit::textChanged, this, &MainWindow::onMarkdownSourceChanged);

    // 粘贴
    connect(m_pasteShortcut, &QShortcut::activated, this, &MainWindow::onPasteImage);

    // 【修复问题3】正确连接 PromptBar 信号
    connect(m_promptBar, &PromptBar::recognizeRequested, this, &MainWindow::onPromptBarRecognize);
    connect(m_promptBar, &PromptBar::autoRecognizeRequested, this, &MainWindow::onPromptBarAutoRecognize);
}

// --- 公共接口实现 ---

void MainWindow::setImage(const QImage& image) {
    m_imageView->setImage(image);
}

void MainWindow::setRecognitionResult(const QString& markdown) {
    m_markdownSource->setPlainText(markdown);
    m_markdownRenderer->setMarkdown(markdown);
}

void MainWindow::showError(const QString& message) {
    QMessageBox::critical(this, "错误", message);
}

void MainWindow::setBusy(bool busy) {
    m_promptBar->setButtonsBusy(busy);
}

// 【新增】设置提示词
void MainWindow::setPrompt(const QString& prompt) {
    m_promptBar->setPrompt(prompt);
}

void MainWindow::startAreaSelection(const QImage& fullImage) {
    // 获取当前鼠标所在的屏幕，或者主屏幕
    QScreen *screen = QGuiApplication::screenAt(QCursor::pos());
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }

    // 传入 screen 指针
    ScreenshotAreaSelector selector(fullImage, screen, nullptr);

    if (selector.exec() == QDialog::Accepted) {
        QRect rect = selector.selectedRect();
        if (!rect.isNull() && rect.isValid()) {
            emit areaSelected(rect);
        } else {
            emit areaSelected(QRect());
        }
    } else {
        emit areaSelected(QRect());
    }
}

// --- 事件处理 ---
void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // 如果按下 ESC 键，调用 close()
    // 这会触发 closeEvent，进而执行 hide() 隐藏窗口
    if (event->key() == Qt::Key_Escape) {
        close();
        event->accept(); // 事件已处理，不再向下传递
    } else {
        // 其他按键按默认方式处理
        QMainWindow::keyPressEvent(event);
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    event->ignore();
    hide();
}

// --- 内部槽 ---

void MainWindow::onPasteImage() {
    QWidget* focus = QApplication::focusWidget();
    if (qobject_cast<QLineEdit*>(focus) || qobject_cast<QPlainTextEdit*>(focus)) {
        if (auto* edit = qobject_cast<QPlainTextEdit*>(focus)) edit->paste();
        else if (auto* lineEdit = qobject_cast<QLineEdit*>(focus)) lineEdit->paste();
        return;
    }

    QClipboard* clipboard = QApplication::clipboard();
    const QMimeData* mimeData = clipboard->mimeData();
    if (mimeData->hasImage()) {
        QImage image = clipboard->image();
        if (!image.isNull()) emit imagePasted(image);
    }
}

void MainWindow::onMarkdownSourceChanged() {
    m_markdownRenderer->setMarkdown(m_markdownSource->toPlainText());
}

// 【新增】处理 PromptBar 信号转发
void MainWindow::onPromptBarRecognize() {
    // 发射信号，带上当前的提示词和图片 Base64
    emit recognizeRequested(m_promptBar->prompt(), m_imageView->currentBase64());
}

void MainWindow::onPromptBarAutoRecognize(const QString& prompt) {
    // 自动识别时，PromptBar 已经更新了内部的 prompt，直接读取即可
    emit recognizeRequested(prompt, m_imageView->currentBase64());
}
