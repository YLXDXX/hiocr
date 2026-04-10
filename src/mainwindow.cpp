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
#include <QCheckBox>
#include <QHBoxLayout>
#include <QToolBar>
#include <QToolButton>


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
    // 1. 创建顶部工具栏替代菜单栏
    m_mainToolBar = new QToolBar("MainToolBar", this);
    addToolBar(Qt::TopToolBarArea, m_mainToolBar);

    // 设置工具栏样式：不显示图标，只显示文字，且按钮不突出（像菜单一样）
    m_mainToolBar->setToolButtonStyle(Qt::ToolButtonTextOnly);
    m_mainToolBar->setMovable(false); // 禁止拖动
    m_mainToolBar->setFloatable(false);

    // 统一设置扁平样式，让 QToolButton 看起来像菜单标题
    m_mainToolBar->setStyleSheet(R"(
        QToolBar { spacing: 2px; padding: 0px; background: palette(window); border-bottom: 1px solid palette(mid); }
        QToolButton { border: none; padding: 4px 6px; border-radius: 3px; background: transparent; }
        QToolButton::menu-indicator { image: none; }
        QToolButton:hover { background: palette(highlight); color: palette(highlightedText); }
    )");

    // 辅助函数：创建菜单按钮
    auto createMenuButton = [this](const QString& text, QMenu* menu) -> QToolButton* {
        QToolButton* btn = new QToolButton(this);
        btn->setText(text);
        btn->setMenu(menu);
        btn->setPopupMode(QToolButton::InstantPopup); // 点击立即弹出菜单
        return btn;
    };

    // --- 2. 文件菜单 ---
    QMenu* fileMenu = new QMenu(this);
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

        m_mainToolBar->addWidget(createMenuButton("文件", fileMenu));

        // --- 3. 工具菜单 ---
        QMenu* toolsMenu = new QMenu(this);
        // 【新增】复制当前图片选项
        m_copyImageAction = toolsMenu->addAction("复制当前图片", this, &MainWindow::onCopyCurrentImage);
        m_copyImageAction->setEnabled(false); // 初始禁用

        m_mainToolBar->addWidget(createMenuButton("工具", toolsMenu));

        // --- 4. 识别服务菜单 ---
        QMenu* serviceMenu = new QMenu(this);

        // 服务选择控件（嵌入菜单中）
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
        m_serviceToggleBtn = new QPushButton("未启动");
        m_serviceToggleBtn->setStyleSheet("QPushButton { border: 1px solid palette(mid); padding: 0 4px; }"); // 按钮样式微调
        connect(m_serviceToggleBtn, &QPushButton::clicked, this, [this](){
            QString id = m_serviceSelector->currentData().toString();
            emit serviceToggleRequested(id);
        });
        layout->addWidget(m_serviceToggleBtn);

        QWidgetAction* serviceWidgetAction = new QWidgetAction(serviceMenu);
        serviceWidgetAction->setDefaultWidget(serviceWidget);
        serviceMenu->addAction(serviceWidgetAction);
        serviceMenu->addSeparator();

        m_stopAllServicesAction = serviceMenu->addAction("停止所有服务", this, [this](){ emit stopServiceRequested(); });
        m_stopAllServicesAction->setEnabled(false);

        m_mainToolBar->addWidget(createMenuButton("识别服务", serviceMenu));

        // --- 5. 设置菜单 ---
        QMenu* settingsMenu = new QMenu(this);
        settingsMenu->addAction("首选项", this, [this](){ emit settingsTriggered(); });

        m_mainToolBar->addWidget(createMenuButton("设置", settingsMenu));

        // --- 6. 脚本控制工具栏 (紧接着设置) ---
        // 添加一个分隔符，视觉上区分菜单和工具
        m_mainToolBar->addSeparator();

        QWidget* scriptWidget = new QWidget();
        QHBoxLayout* scriptLayout = new QHBoxLayout(scriptWidget);
        scriptLayout->setContentsMargins(0, 0, 0, 0);
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

        // 将脚本控件添加到工具栏
        m_mainToolBar->addWidget(scriptWidget);

        // --- 7. 连接信号 ---
        SettingsManager* s = SettingsManager::instance();
        connect(m_scriptGlobalCheck, &QCheckBox::toggled, s, &SettingsManager::setAutoExternalProcessBeforeCopy);
        connect(m_scriptTextCheck, &QCheckBox::toggled, s, &SettingsManager::setTextProcessorEnabled);
        connect(m_scriptFormulaCheck, &QCheckBox::toggled, s, &SettingsManager::setFormulaProcessorEnabled);
        connect(m_scriptTableCheck, &QCheckBox::toggled, s, &SettingsManager::setTableProcessorEnabled);
        connect(m_scriptPureMathCheck, &QCheckBox::toggled, s, &SettingsManager::setPureMathProcessorEnabled);

        updateScriptCheckboxes();
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
    connect(m_imageView, &ImageViewWidget::imageChanged, this, &MainWindow::updateCopyImageActionState);
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

void MainWindow::onCopyCurrentImage()
{
    if (!m_imageView || !m_imageView->hasImage()) {
        return;
    }

    QImage image = m_imageView->currentImage();
    if (!image.isNull()) {
        // 将图片写入系统剪贴板
        QApplication::clipboard()->setImage(image);
        statusBar()->showMessage("图片已复制到剪贴板", 3000);
    } else {
        statusBar()->showMessage("无法复制：图片数据无效", 3000);
    }
}

void MainWindow::updateCopyImageActionState()
{
    if (m_copyImageAction) {
        // 根据是否有图片来设置菜单项是否可用
        bool hasImage = m_imageView && m_imageView->hasImage();
        m_copyImageAction->setEnabled(hasImage);
    }
}


void MainWindow::setStreamingMode(bool streaming)
{
    m_isStreaming = streaming;

    // 关键优化：流式传输时，断开 Markdown 渲染器的自动更新连接
    // 因为 WebEngineView 渲染非常慢，每次追加文本都渲染会卡死 UI
    if (streaming) {
        disconnect(m_markdownSource, &QPlainTextEdit::textChanged,
                   this, &MainWindow::onMarkdownSourceChanged);
    } else {
        // 流式传输结束，恢复连接并立即渲染一次最终结果
        connect(m_markdownSource, &QPlainTextEdit::textChanged,
                this, &MainWindow::onMarkdownSourceChanged);

        // 渲染最终完整文本
        m_markdownRenderer->setMarkdown(m_markdownSource->toPlainText());
    }
}

void MainWindow::appendRecognitionResult(const QString& delta)
{
    if (!m_markdownSource) return;

    // 移动光标到文本末尾
    QTextCursor cursor = m_markdownSource->textCursor();
    cursor.movePosition(QTextCursor::End);

    // 插入增量文本
    cursor.insertText(delta);

    // 保持滚动
    m_markdownSource->ensureCursorVisible();

    // 【重要】此时不要调用渲染器，因为 setStreamingMode(true) 已经断开了连接
    // 这样纯文本编辑器的更新非常快，不会卡顿
}
