#include "mainwindow.h"
#include "imageviewwidget.h"
#include "markdownrenderer.h"
#include "markdownsourceeditor.h"
#include "promptbar.h"
#include "imageprocessor.h"
#include "screenshotareaselector.h"
#include "settingsmanager.h"
#include "copyprocessor.h"
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
#include <QLabel>
#include <QComboBox>
#include <QWidgetAction>
#include <QCheckBox>  // 【新增】
#include <QHBoxLayout> // 【新增】

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

    QSplitter* leftSplitter = new QSplitter(Qt::Vertical);
    mainSplitter->addWidget(leftSplitter);

    m_imageView = new ImageViewWidget(this);
    leftSplitter->addWidget(m_imageView);

    m_markdownRenderer = new MarkdownRenderer(this);
    leftSplitter->addWidget(m_markdownRenderer);
    leftSplitter->setSizes({400, 400});

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
        fileMenu->addAction("截图", this, [this](){ emit screenshotRequested(); });
        fileMenu->addSeparator();
        fileMenu->addAction("退出", qApp, &QApplication::quit);

        QMenu* toolsMenu = menuBar->addMenu("工具");

        // 【修改】识别服务菜单
        QMenu* serviceMenu = menuBar->addMenu("识别服务");

        QWidget* serviceWidget = new QWidget();
        QHBoxLayout* layout = new QHBoxLayout(serviceWidget);
        layout->setContentsMargins(5, 0, 5, 0);

        layout->addWidget(new QLabel("当前服务:"));

        m_serviceSelector = new QComboBox();
        connect(m_serviceSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index){
            if (index < 0) return;
            QString id = m_serviceSelector->itemData(index).toString();
            emit serviceSelected(id);
        });
        layout->addWidget(m_serviceSelector);

        m_serviceToggleBtn = new QPushButton("未启动"); // 默认文字
        connect(m_serviceToggleBtn, &QPushButton::clicked, this, [this](){
            QString id = m_serviceSelector->currentData().toString();
            emit serviceToggleRequested(id);
        });
        layout->addWidget(m_serviceToggleBtn);

        QWidgetAction* actionWidget = new QWidgetAction(serviceMenu);
        actionWidget->setDefaultWidget(serviceWidget);
        serviceMenu->addAction(actionWidget);

        serviceMenu->addSeparator();

        m_stopAllServicesAction = serviceMenu->addAction("停止所有服务", this, [this](){
            emit stopServiceRequested();
        });
        m_stopAllServicesAction->setEnabled(false);

        QMenu* settingsMenu = menuBar->addMenu("设置");
        settingsMenu->addAction("首选项", this, [this](){ emit settingsTriggered(); });


        // --- 新增：右侧脚本控制工具栏 ---
        QWidget* scriptWidget = new QWidget();
        QHBoxLayout* scriptLayout = new QHBoxLayout(scriptWidget);
        scriptLayout->setContentsMargins(5, 0, 5, 0);
        scriptLayout->setSpacing(5);

        m_scriptGlobalCheck = new QCheckBox("脚本");
        m_scriptGlobalCheck->setToolTip("全局启用/禁用复制前自动调用脚本");
        scriptLayout->addWidget(m_scriptGlobalCheck);

        scriptLayout->addWidget(new QLabel("|"));

        m_scriptTextCheck = new QCheckBox("文字");
        m_scriptTextCheck->setToolTip("启用文字脚本");
        scriptLayout->addWidget(m_scriptTextCheck);

        m_scriptFormulaCheck = new QCheckBox("公式");
        m_scriptFormulaCheck->setToolTip("启用公式脚本");
        scriptLayout->addWidget(m_scriptFormulaCheck);

        m_scriptTableCheck = new QCheckBox("表格");
        m_scriptTableCheck->setToolTip("启用表格脚本");
        scriptLayout->addWidget(m_scriptTableCheck);

        m_scriptPureMathCheck = new QCheckBox("纯公式");
        m_scriptPureMathCheck->setToolTip("启用纯数学公式脚本 (优先级最高)");
        scriptLayout->addWidget(m_scriptPureMathCheck);

        // 使用 setCornerWidget 将其放在菜单栏右侧
        menuBar->setCornerWidget(scriptWidget, Qt::TopRightCorner);

        // 连接信号
        SettingsManager* s = SettingsManager::instance();

        // 双向绑定：UI -> Settings
        connect(m_scriptGlobalCheck, &QCheckBox::toggled, s, &SettingsManager::setAutoExternalProcessBeforeCopy);
        connect(m_scriptTextCheck, &QCheckBox::toggled, s, &SettingsManager::setTextProcessorEnabled);
        connect(m_scriptFormulaCheck, &QCheckBox::toggled, s, &SettingsManager::setFormulaProcessorEnabled);
        connect(m_scriptTableCheck, &QCheckBox::toggled, s, &SettingsManager::setTableProcessorEnabled);
        connect(m_scriptPureMathCheck, &QCheckBox::toggled, s, &SettingsManager::setPureMathProcessorEnabled);

        // 初始化 UI 状态
        updateScriptCheckboxes();

        // 监听设置变化更新 UI (例如在设置对话框修改后)
        connect(s, &SettingsManager::autoExternalProcessBeforeCopyChanged, this, &MainWindow::updateScriptCheckboxes);
}

void MainWindow::setupConnections()
{
    connect(m_markdownSource, &QPlainTextEdit::textChanged, this, &MainWindow::onMarkdownSourceChanged);
    connect(m_pasteShortcut, &QShortcut::activated, this, &MainWindow::onPasteImage);
    connect(m_promptBar, &PromptBar::recognizeRequested, this, &MainWindow::onPromptBarRecognize);
    connect(m_promptBar, &PromptBar::autoRecognizeRequested, this, [this](const QString& prompt, ContentType type){
        QString base64 = m_imageView->currentBase64();
        emit typedRecognizeRequested(prompt, base64, type);
    });
}

void MainWindow::setImage(const QImage& image) { m_imageView->setImage(image); }
void MainWindow::setRecognitionResult(const QString& markdown) { m_markdownSource->setPlainText(markdown); m_markdownRenderer->setMarkdown(markdown); }
void MainWindow::showError(const QString& message) { QMessageBox::critical(this, "错误", message); }
void MainWindow::setBusy(bool busy) { m_promptBar->setButtonsBusy(busy); }
void MainWindow::setPrompt(const QString& prompt) { m_promptBar->setPrompt(prompt); }

void MainWindow::setCurrentPrompts(const QString& text, const QString& formula, const QString& table)
{
    if (m_promptBar) {
        m_promptBar->setCurrentPrompts(text, formula, table);
    }
}

void MainWindow::startAreaSelection(const QImage& fullImage) {
    QScreen *screen = QGuiApplication::screenAt(QCursor::pos());
    if (!screen) screen = QGuiApplication::primaryScreen();
    ScreenshotAreaSelector selector(fullImage, screen, nullptr);
    if (selector.exec() == QDialog::Accepted) {
        QRect rect = selector.selectedRect();
        if (!rect.isNull() && rect.isValid()) emit areaSelected(rect);
        else emit areaSelected(QRect());
    } else {
        emit areaSelected(QRect());
    }
}

void MainWindow::updateServiceSelector(const QList<ServiceProfile>& profiles, const QString& currentId)
{
    m_serviceSelector->blockSignals(true);
    m_serviceSelector->clear();

    // 【新增】插入“全局默认”选项
    m_serviceSelector->addItem("全局默认 (远程)", ""); // Data 设为空字符串

    for (const auto& p : profiles) {
        m_serviceSelector->addItem(p.name, p.id);
    }

    int idx = m_serviceSelector->findData(currentId);
    if (idx >= 0) m_serviceSelector->setCurrentIndex(idx);
    else m_serviceSelector->setCurrentIndex(0); // 默认选中“全局默认”

    m_serviceSelector->blockSignals(false);
}

void MainWindow::updateServiceControlButton(const QString& id, bool isRunning)
{
    // 【修改】当选中的是全局默认时，按钮状态处理
    if (id.isEmpty()) {
        // 选中全局默认时，如果该服务正在运行，显示运行中；否则显示远程状态
        // 这里的逻辑是：用户选择全局默认，意味着使用远程，不需要按钮管理本地进程
        // 或者该远程服务恰好也是本地服务之一（比如用户没选中它），但此处 UI 只负责显示当前选中的行为

        // 判断当前 ComboBox 选中的是不是空 ID
        if (m_serviceSelector->currentData().toString().isEmpty()) {
            m_serviceToggleBtn->setText("远程服务");
            m_serviceToggleBtn->setEnabled(false);
            m_serviceToggleBtn->setStyleSheet("");
            return;
        }
    }

    // 只有当选中的服务与当前操作的服务一致时才更新 UI
    if (m_serviceSelector->currentData().toString() == id) {
        m_serviceToggleBtn->setEnabled(true);
        m_serviceToggleBtn->setText(isRunning ? "已启动" : "未启动");
        // 简单的颜色区分
        m_serviceToggleBtn->setStyleSheet(isRunning ? "background-color: #90EE90; color: black;" : "");
    }
}

void MainWindow::updateStopAllAction(int runningCount)
{
    m_stopAllServicesAction->setEnabled(runningCount > 0);
    m_stopAllServicesAction->setText(QString("停止所有服务 (运行中: %1)").arg(runningCount));
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) { close(); event->accept(); }
    else QMainWindow::keyPressEvent(event);
}

void MainWindow::closeEvent(QCloseEvent *event) { event->ignore(); hide(); }

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

void MainWindow::onMarkdownSourceChanged() { m_markdownRenderer->setMarkdown(m_markdownSource->toPlainText()); }
void MainWindow::onPromptBarRecognize()
{
    QString currentPrompt = m_promptBar->prompt();
    emit recognizeRequested(currentPrompt, m_imageView->currentBase64());
}
void MainWindow::onPromptBarAutoRecognize(const QString& prompt) { emit recognizeRequested(prompt, m_imageView->currentBase64()); }

// 【新增】手动触发脚本处理的实现
void MainWindow::onManualProcessorTriggered(ContentType type)
{
    QString currentText = currentMarkdownSource();
    if (currentText.isEmpty()) {
        statusBar()->showMessage("内容为空，无法处理。", 3000);
        return;
    }

    // 发送信号给 AppController，让 Controller 处理具体逻辑
    emit manualProcessRequested(type);
}

void MainWindow::updateScriptCheckboxes()
{
    SettingsManager* s = SettingsManager::instance();

    // 防止信号循环
    m_scriptGlobalCheck->blockSignals(true);
    m_scriptTextCheck->blockSignals(true);
    m_scriptFormulaCheck->blockSignals(true);
    m_scriptTableCheck->blockSignals(true);
    m_scriptPureMathCheck->blockSignals(true);

    m_scriptGlobalCheck->setChecked(s->autoExternalProcessBeforeCopy());
    m_scriptTextCheck->setChecked(s->textProcessorEnabled());
    m_scriptFormulaCheck->setChecked(s->formulaProcessorEnabled());
    m_scriptTableCheck->setChecked(s->tableProcessorEnabled());
    m_scriptPureMathCheck->setChecked(s->pureMathProcessorEnabled());

    m_scriptGlobalCheck->blockSignals(false);
    m_scriptTextCheck->blockSignals(false);
    m_scriptFormulaCheck->blockSignals(false);
    m_scriptTableCheck->blockSignals(false);
    m_scriptPureMathCheck->blockSignals(false);
}

QString MainWindow::currentMarkdownSource() const
{
    return m_markdownSource ? m_markdownSource->toPlainText() : QString();
}
bool MainWindow::hasImage() const
{
    return m_imageView && m_imageView->hasImage();
}

void MainWindow::setRecognizeType(ContentType type)
{
    if (m_copyBar) {
        m_copyBar->setOriginalRecognizeType(type);
    }
}
