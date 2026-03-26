#include "mainwindow.h"
#include "imageviewwidget.h"
#include "markdownrenderer.h"
#include "markdownsourceeditor.h"
#include "promptbar.h"
#include "imageprocessor.h"
#include "screenshotareaselector.h"
#include "settingsmanager.h"
#include "markdowncopybar.h"

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
    setupMenuBar();
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

    // 右侧布局
    QSplitter* rightSplitter = new QSplitter(Qt::Vertical);
    mainSplitter->addWidget(rightSplitter);

    m_promptBar = new PromptBar(this);
    rightSplitter->addWidget(m_promptBar);

    QWidget* markdownContainer = new QWidget(this);
    QVBoxLayout* mdLayout = new QVBoxLayout(markdownContainer);
    mdLayout->setContentsMargins(0, 0, 0, 0);
    mdLayout->setSpacing(2);

    m_markdownSource = new MarkdownSourceEditor(this);
    m_copyBar = new MarkdownCopyBar(this);
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

        // 【新增】停止服务菜单项
        m_stopServiceAction = toolsMenu->addAction("停止识别服务", this, [this](){
            emit stopServiceRequested();
        });
        m_stopServiceAction->setEnabled(false); // 默认禁用

        // --- 设置菜单 ---
        QMenu* settingsMenu = menuBar->addMenu("设置");
        settingsMenu->addAction("首选项", this, [this](){
            emit settingsTriggered();
        });
}

void MainWindow::setupConnections()
{
    connect(m_markdownSource, &QPlainTextEdit::textChanged, this, &MainWindow::onMarkdownSourceChanged);
    connect(m_pasteShortcut, &QShortcut::activated, this, &MainWindow::onPasteImage);
    connect(m_promptBar, &PromptBar::recognizeRequested, this, &MainWindow::onPromptBarRecognize);
    connect(m_promptBar, &PromptBar::autoRecognizeRequested, this, &MainWindow::onPromptBarAutoRecognize);
}

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

void MainWindow::setPrompt(const QString& prompt) {
    m_promptBar->setPrompt(prompt);
}

void MainWindow::startAreaSelection(const QImage& fullImage) {
    QScreen *screen = QGuiApplication::screenAt(QCursor::pos());
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
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

// 【新增】实现状态更新槽函数
void MainWindow::updateStopServiceAction(bool isRunning)
{
    if (m_stopServiceAction) {
        m_stopServiceAction->setEnabled(isRunning);
        // 可选：更新文本以提示状态
        m_stopServiceAction->setText(isRunning ? "停止识别服务 (运行中)" : "停止识别服务 (未运行)");
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        close();
        event->accept();
    } else {
        QMainWindow::keyPressEvent(event);
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    event->ignore();
    hide();
}

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

void MainWindow::onPromptBarRecognize() {
    emit recognizeRequested(m_promptBar->prompt(), m_imageView->currentBase64());
}

void MainWindow::onPromptBarAutoRecognize(const QString& prompt) {
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

    QProcess* process = new QProcess(this);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::onExternalProcessFinished);

    process->startCommand(command);
    process->write(currentText.toUtf8());
    process->closeWriteChannel();
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
        QString result = QString::fromUtf8(process->readAllStandardOutput());
        if (result.isEmpty()) {
            QMessageBox::warning(this, "提示", "外部程序未返回任何内容。");
        } else {
            QApplication::clipboard()->setText(result);
            statusBar()->showMessage("已通过外部程序处理并复制到剪贴板", 3000);
        }
    }
    process->deleteLater();
}


// 【新增】实现统一复制接口
void MainWindow::copyToClipboard(const QString& text)
{
    if (text.isEmpty()) return;

    if (m_copyBar) {
        // 复用 MarkdownCopyBar 的逻辑，它会自动处理“是否调用外部程序”的判断
        m_copyBar->executeCopy(text);
    } else {
        // 保底逻辑
        QApplication::clipboard()->setText(text);
    }
}
