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
#include <QStatusBar>
#include <QProcess>


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

        // --- 工具菜单 ---
        QMenu* toolsMenu = menuBar->addMenu("工具");
        QAction* extAct = toolsMenu->addAction("外部程序处理并复制", this, &MainWindow::onExternalProcessTriggered);
        extAct->setShortcut(QKeySequence("Ctrl+Shift+E")); // 可选快捷键

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


void MainWindow::onExternalProcessTriggered()
{
    QString command = SettingsManager::instance()->externalProcessorCommand();
    if (command.isEmpty()) {
        QMessageBox::warning(this, "提示", "未配置外部处理程序。\n请在 设置 -> 外部处理程序 中配置命令。");
        return;
    }

    QString currentText = m_markdownSource->toPlainText();
    if (currentText.isEmpty()) {
        QMessageBox::information(this, "提示", "内容为空，无需处理。");
        return;
    }

    // 创建进程
    QProcess* process = new QProcess(this);

    // 连接结束信号
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::onExternalProcessFinished);

    // 启动命令 (Qt6 推荐使用 startCommand，它会自动处理参数分割)
    // 如果使用 Qt5，请使用 process->start(command, QProcess::Unbuffered);
    process->startCommand(command);

    // 写入输入
    process->write(currentText.toUtf8());
    process->closeWriteChannel(); // 发送 EOF，通知外部程序输入结束

    // 为了更好的用户体验，可以在这里显示一个等待状态，但简单起见我们先同步处理
    // 注意：process->waitForFinished() 会阻塞 UI，如果外部程序运行时间长，
    // 建议在 onExternalProcessFinished 中处理结果。
    // 这里我们使用异步方式，不阻塞 UI。
}

void MainWindow::onExternalProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QProcess* process = qobject_cast<QProcess*>(sender());
    if (!process) return;

    if (exitStatus != QProcess::NormalExit || exitCode != 0) {
        QString error = process->readAllStandardError();
        if (error.isEmpty()) error = "进程异常退出或返回非零代码。";
        QMessageBox::critical(this, "处理失败", error);
    } else {
        // 读取标准输出
        QString result = QString::fromUtf8(process->readAllStandardOutput());

        if (result.isEmpty()) {
            QMessageBox::warning(this, "提示", "外部程序未返回任何内容。");
        } else {
            // 【关键】将结果复制到剪贴板，不修改源码编辑器
            QApplication::clipboard()->setText(result);
            // 可选：显示提示
            statusBar()->showMessage("已通过外部程序处理并复制到剪贴板", 3000);
        }
    }

    process->deleteLater(); // 清理进程对象
}
