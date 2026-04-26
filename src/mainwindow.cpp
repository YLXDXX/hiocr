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
#include "historymanager.h"
#include "constants.h"

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

#include <QDialog>
#include <QListWidget>
#include <QDialogButtonBox>
#include <QSpinBox>



MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setupUi();
    setupMenuBar();
    setupConnections();
}

MainWindow::~MainWindow() {}

void MainWindow::setupUi()
{
    setWindowTitle("HiOCR");
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
    // 1. 创建顶部工具栏
    m_mainToolBar = new QToolBar("MainToolBar", this);
    addToolBar(Qt::TopToolBarArea, m_mainToolBar);
    m_mainToolBar->setToolButtonStyle(Qt::ToolButtonTextOnly);
    m_mainToolBar->setMovable(false);
    m_mainToolBar->setFloatable(false);
    m_mainToolBar->setStyleSheet(R"(
        QToolBar { spacing: 2px; padding: 0px; background: palette(window); border-bottom: 1px solid palette(mid); }
        QToolButton { border: none; padding: 4px 6px; border-radius: 3px; background: transparent; }
        QToolButton::menu-indicator { image: none; }
        QToolButton:hover { background: palette(highlight); color: palette(highlightedText); }
    )");

    auto createMenuButton = [this](const QString& text, QMenu* menu) -> QToolButton* {
        QToolButton* btn = new QToolButton(this);
        btn->setText(text);
        btn->setMenu(menu);
        btn->setPopupMode(QToolButton::InstantPopup);
        return btn;
    };

    // --- 文件菜单 ---
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

        // 【新增】历史记录菜单项
        QAction* historyAction = fileMenu->addAction("查看历史记录", this, &MainWindow::showHistoryDialog);
        historyAction->setShortcut(QKeySequence("Ctrl+H"));

        fileMenu->addSeparator();
        fileMenu->addAction("退出", qApp, &QApplication::quit);

        m_mainToolBar->addWidget(createMenuButton("文件", fileMenu));

        // --- 工具菜单 ---
        QMenu* toolsMenu = new QMenu(this);
        toolsMenu->addAction("截图", this, [this](){ emit screenshotRequested(); });
        toolsMenu->addSeparator();
        m_copyImageAction = toolsMenu->addAction("复制当前图片", this, &MainWindow::onCopyCurrentImage);
        m_copyImageAction->setEnabled(false);

        toolsMenu->addSeparator();                                    // 【新增】
        m_abortAction = toolsMenu->addAction("强制中断识别", this, [this](){  // 【新增】
            emit abortRequested();
        });
        m_abortAction->setToolTip("中止当前正在进行的识别请求");

        m_mainToolBar->addWidget(createMenuButton("工具", toolsMenu));

        // --- 识别服务菜单 ---
        QMenu* serviceMenu = new QMenu(this);
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
        m_serviceToggleBtn->setStyleSheet("QPushButton { border: 1px solid palette(mid); padding: 0 4px; }");
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

        // --- 设置菜单 ---
        QMenu* settingsMenu = new QMenu(this);
        settingsMenu->addAction("首选项", this, [this](){ emit settingsTriggered(); });
        m_mainToolBar->addWidget(createMenuButton("设置", settingsMenu));

        // --- 脚本控制 ---
        m_mainToolBar->addSeparator();
        QWidget* scriptWidget = new QWidget();
        QHBoxLayout* scriptLayout = new QHBoxLayout(scriptWidget);
        scriptLayout->setContentsMargins(0, 0, 0, 0);
        scriptLayout->setSpacing(5);
        m_scriptGlobalCheck = new QCheckBox("脚本");
        scriptLayout->addWidget(m_scriptGlobalCheck);
        scriptLayout->addWidget(new QLabel("|"));
        m_scriptTextCheck = new QCheckBox("文字");
        scriptLayout->addWidget(m_scriptTextCheck);
        m_scriptFormulaCheck = new QCheckBox("公式");
        scriptLayout->addWidget(m_scriptFormulaCheck);
        m_scriptTableCheck = new QCheckBox("表格");
        scriptLayout->addWidget(m_scriptTableCheck);
        m_scriptPureMathCheck = new QCheckBox("纯公式");
        scriptLayout->addWidget(m_scriptPureMathCheck);
        m_mainToolBar->addWidget(scriptWidget);

        // 连接信号
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
void MainWindow::setPrompt(const QString& prompt) { m_promptBar->setPrompt(prompt); }

void MainWindow::showError(const QString& message)
{
    // 静默模式下窗口隐藏，不弹出错误对话框，错误由通知反馈
    if (!isVisible()) return;
    QMessageBox::critical(this, "错误", message);
}

void MainWindow::setBusy(bool busy)
{
    // 这会将 PromptBar 上的所有按钮置灰
    if (m_promptBar) {
        m_promptBar->setButtonsBusy(busy);
    }
}

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
    if (!m_markdownRenderer) return;

    if (streaming) {
        // 进入流式模式：
        // 1. 清空源码编辑器
        m_markdownSource->clear();

        // 2. 通知渲染器开始流式模式（启动定时器）
        m_markdownRenderer->startStreaming();

        // 3. 断开源码编辑器的自动渲染连接（防止干扰）
        disconnect(m_markdownSource, &QPlainTextEdit::textChanged,
                   this, &MainWindow::onMarkdownSourceChanged);
    } else {
        // 结束流式模式：
        // 1. 通知渲染器停止（它会立即渲染最终结果）
        m_markdownRenderer->stopStreaming();

        // 2. 恢复源码编辑器的自动渲染连接
        connect(m_markdownSource, &QPlainTextEdit::textChanged,
                this, &MainWindow::onMarkdownSourceChanged);
    }
}

void MainWindow::appendRecognitionResult(const QString& delta)
{
    if (!m_markdownSource) return;

    // 1. 更新源码编辑器（保持即时显示，开销很小）
    QTextCursor cursor = m_markdownSource->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(delta);
    m_markdownSource->ensureCursorVisible();

    // 2. 【关键】将增量数据推送给渲染器的缓冲区
    if (m_markdownRenderer) {
        m_markdownRenderer->appendStreamContent(delta);
    }
}


void MainWindow::showHistoryDialog()
{
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle("识别历史记录");
    dialog->resize(900, 600);

    QVBoxLayout* mainLayout = new QVBoxLayout(dialog);

    // --- 记录列表 ---
    QListWidget* listWidget = new QListWidget(dialog);
    listWidget->setViewMode(QListView::ListMode);
    listWidget->setIconSize(QSize(250, 250));
    listWidget->setResizeMode(QListView::Adjust);
    listWidget->setAlternatingRowColors(true);
    listWidget->setWordWrap(true);
    listWidget->setStyleSheet("QListWidget::item { padding: 10px; border-bottom: 1px solid #ddd; }");
    mainLayout->addWidget(listWidget);

    // --- 【新增】分页控件 ---
    QWidget* pageWidget = new QWidget(dialog);
    QHBoxLayout* pageLayout = new QHBoxLayout(pageWidget);
    pageLayout->setContentsMargins(0, 4, 0, 4);

    QPushButton* prevPageBtn = new QPushButton("上一页");
    QPushButton* nextPageBtn = new QPushButton("下一页");
    QLabel* pageInfoLabel = new QLabel();
    pageInfoLabel->setAlignment(Qt::AlignCenter);

    // 跳转控件
    QLabel* jumpLabel = new QLabel("跳转到:");
    QSpinBox* jumpSpin = new QSpinBox();
    jumpSpin->setMinimum(1);
    QPushButton* jumpBtn = new QPushButton("跳转");

    pageLayout->addWidget(prevPageBtn);
    pageLayout->addStretch();
    pageLayout->addWidget(pageInfoLabel);
    pageLayout->addStretch();
    pageLayout->addWidget(jumpLabel);
    pageLayout->addWidget(jumpSpin);
    pageLayout->addWidget(jumpBtn);
    pageLayout->addWidget(nextPageBtn);

    mainLayout->addWidget(pageWidget);

    // --- 底部操作按钮 ---
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Open | QDialogButtonBox::Close, dialog);
    QPushButton* loadBtn = buttons->button(QDialogButtonBox::Open);
    loadBtn->setText("加载到编辑器");

    QPushButton* deleteBtn = buttons->addButton("删除选中", QDialogButtonBox::ActionRole);
    QPushButton* clearBtn = buttons->addButton("清空历史", QDialogButtonBox::ActionRole);

    mainLayout->addWidget(buttons);

    // --- 分页状态 ---
    const int pageSize = Constants::HISTORY_PAGE_SIZE;
    int currentPage = 1;
    int totalCount = 0;
    int totalPages = 1;

    // --- 加载当前页数据 ---
    // 使用 std::function 实现递归 lambda（C++14/17 兼容）
    std::function<void()> loadCurrentPage;
    loadCurrentPage = [&]() {
        // 获取最新的总记录数
        totalCount = HistoryManager::instance()->getTotalCount();
        totalPages = (totalCount == 0) ? 1 : (totalCount + pageSize - 1) / pageSize;

        // 修正当前页码（删除记录后可能超出范围）
        if (currentPage > totalPages) {
            currentPage = totalPages;
        }
        if (currentPage < 1) {
            currentPage = 1;
        }

        // 清空列表
        listWidget->clear();

        // 计算偏移量
        int offset = (currentPage - 1) * pageSize;

        // 从数据库查询当前页记录
        auto records = HistoryManager::instance()->getRecentRecords(pageSize, offset);
        for (const auto& record : records) {
            QListWidgetItem* item = new QListWidgetItem(listWidget);

            // 设置图标
            QPixmap pix(record.cachedImagePath);
            if (!pix.isNull()) {
                item->setIcon(QIcon(pix.scaled(250, 250, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
            } else {
                item->setIcon(QIcon::fromTheme("image-missing"));
            }

            // 解析类型
            QString typeStr = "文字";
            if (record.recognitionType == ContentType::Formula) typeStr = "公式";
            else if (record.recognitionType == ContentType::Table) typeStr = "表格";

            // 格式化显示文本
            QString header = QString("[%1] %2").arg(record.timestamp.toString("MM-dd hh:mm")).arg(typeStr);

            QString preview = record.resultText;
            preview.replace("\n", " ");
            if (preview.length() > 80) {
                preview = preview.left(80) + "...";
            }

            item->setText(QString("%1\n%2").arg(header).arg(preview));

            item->setToolTip(QString("时间: %1\n类型: %2\n\n%3")
            .arg(record.timestamp.toString("yyyy-MM-dd hh:mm:ss"))
            .arg(typeStr)
            .arg(record.resultText));

            item->setData(Qt::UserRole, record.id);
        }

        // 更新分页控件状态
        prevPageBtn->setEnabled(currentPage > 1);
        nextPageBtn->setEnabled(currentPage < totalPages);

        // 更新页码信息
        if (totalCount == 0) {
            pageInfoLabel->setText("暂无记录");
        } else {
            pageInfoLabel->setText(QString("第 %1 / %2 页  (共 %3 条)")
            .arg(currentPage)
            .arg(totalPages)
            .arg(totalCount));
        }

        // 更新跳转控件
        jumpSpin->setMaximum(totalPages);
        jumpSpin->setValue(currentPage);
        jumpSpin->setEnabled(totalPages > 1);
        jumpBtn->setEnabled(totalPages > 1);
    };

    // --- 首次加载 ---
    loadCurrentPage();

    // --- 翻页信号 ---
    connect(prevPageBtn, &QPushButton::clicked, this, [&]() {
        if (currentPage > 1) {
            currentPage--;
            loadCurrentPage();
        }
    });

    connect(nextPageBtn, &QPushButton::clicked, this, [&]() {
        if (currentPage < totalPages) {
            currentPage++;
            loadCurrentPage();
        }
    });

    connect(jumpBtn, &QPushButton::clicked, this, [&]() {
        int targetPage = jumpSpin->value();
        if (targetPage >= 1 && targetPage <= totalPages) {
            currentPage = targetPage;
            loadCurrentPage();
        }
    });

    // 跳转框回车触发跳转
    connect(jumpSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [&](int value) {
        // 只在用户通过键盘回车确认时跳转，不在这里自动跳转
        // 避免每次值变化都触发加载
        Q_UNUSED(value);
    });

    // --- 操作按钮信号 ---

    // 加载按钮
    connect(loadBtn, &QPushButton::clicked, this, [this, listWidget, dialog](){
        auto currentItem = listWidget->currentItem();
        if (currentItem) {
            int recordId = currentItem->data(Qt::UserRole).toInt();
            emit loadHistoryRecordRequested(recordId);
            dialog->close();
        }
    });

    // 删除按钮
    connect(deleteBtn, &QPushButton::clicked, this, [&]() {
        auto currentItem = listWidget->currentItem();
        if (currentItem) {
            int recordId = currentItem->data(Qt::UserRole).toInt();
            HistoryManager::instance()->deleteRecord(recordId);
            // 不关闭对话框，刷新当前页
            loadCurrentPage();
        }
    });

    // 清空按钮
    connect(clearBtn, &QPushButton::clicked, this, [&]() {
        if (QMessageBox::question(dialog, "确认", "确定要清空所有历史记录吗？") == QMessageBox::Yes) {
            HistoryManager::instance()->clearAll();
            currentPage = 1;
            loadCurrentPage();
        }
    });

    // 双击加载
    connect(listWidget, &QListWidget::itemDoubleClicked, this, [this, listWidget, dialog](QListWidgetItem* item){
        int recordId = item->data(Qt::UserRole).toInt();
        emit loadHistoryRecordRequested(recordId);
        dialog->close();
    });

    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::close);

    dialog->exec();
    dialog->deleteLater();
}

// 【新增】返回当前显示的图片 (用于 AppController 保存历史)
QImage MainWindow::currentImage() const
{
    if (m_imageView) {
        return m_imageView->currentImage();
    }
    return QImage();
}

void MainWindow::setCurrentServiceName(const QString& name)
{
    // 同步到渲染器（通过 Bridge → JS 判断渲染模式）
    if (m_markdownRenderer) {
        m_markdownRenderer->setServiceName(name);
    }

    // 同步到复制栏（判断复制行为 + 传递 --mode-name）
    if (m_copyBar) {
        m_copyBar->setCurrentServiceName(name);
    }
}
