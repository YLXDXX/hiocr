// src/mainwindow_impl.h
#ifndef MAINWINDOW_IMPL_H
#define MAINWINDOW_IMPL_H

#include "mainwindow.h"

#include <QGuiApplication>
#include <QScreen>
#include <QPixmap>
#include <QPushButton>
#include <QPlainTextEdit>

// --- Implementation ---

inline MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setupUi();
    setupMenuBar();
    setupConnections();
}

inline MainWindow::~MainWindow() {}

inline void MainWindow::setupUi()
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

inline void MainWindow::setupMenuBar()
{
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

        QAction* historyAction = fileMenu->addAction("查看历史记录", this, &MainWindow::showHistoryDialog);
        historyAction->setShortcut(QKeySequence("Ctrl+H"));

        fileMenu->addSeparator();
        fileMenu->addAction("退出", qApp, &QApplication::quit);

        m_mainToolBar->addWidget(createMenuButton("文件", fileMenu));

        QMenu* toolsMenu = new QMenu(this);
        toolsMenu->addAction("截图", this, [this](){ emit screenshotRequested(); });
        toolsMenu->addSeparator();
        m_copyImageAction = toolsMenu->addAction("复制当前图片", this, &MainWindow::onCopyCurrentImage);
        m_copyImageAction->setEnabled(false);

        toolsMenu->addSeparator();
        m_abortAction = toolsMenu->addAction("强制中断识别", this, [this](){
            emit abortRequested();
        });
        m_abortAction->setToolTip("中止当前正在进行的识别请求");

        m_mainToolBar->addWidget(createMenuButton("工具", toolsMenu));

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

        QMenu* settingsMenu = new QMenu(this);
        settingsMenu->addAction("首选项", this, [this](){ emit settingsTriggered(); });
        m_mainToolBar->addWidget(createMenuButton("设置", settingsMenu));

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
        scriptLayout->addWidget(new QLabel("|"));
        m_formatterToolCheck = new QCheckBox("格式化");
        scriptLayout->addWidget(m_formatterToolCheck);
        m_mainToolBar->addWidget(scriptWidget);

        SettingsManager* s = SettingsManager::instance();
        connect(m_scriptGlobalCheck, &QCheckBox::toggled, s, &SettingsManager::setAutoExternalProcessBeforeCopy);
        connect(m_scriptTextCheck, &QCheckBox::toggled, s, &SettingsManager::setTextProcessorEnabled);
        connect(m_scriptFormulaCheck, &QCheckBox::toggled, s, &SettingsManager::setFormulaProcessorEnabled);
        connect(m_scriptTableCheck, &QCheckBox::toggled, s, &SettingsManager::setTableProcessorEnabled);
        connect(m_scriptPureMathCheck, &QCheckBox::toggled, s, &SettingsManager::setPureMathProcessorEnabled);
        connect(m_formatterToolCheck, &QCheckBox::toggled, s, &SettingsManager::setFormatterEnabled);

        updateScriptCheckboxes();
        connect(s, &SettingsManager::autoExternalProcessBeforeCopyChanged, this, &MainWindow::updateScriptCheckboxes);
        connect(s, &SettingsManager::formatterEnabledChanged, this, &MainWindow::updateScriptCheckboxes);
}

inline void MainWindow::setupConnections()
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

inline void MainWindow::setImage(const QImage& image) { m_imageView->setImage(image); }
inline void MainWindow::setRecognitionResult(const QString& markdown) { m_markdownSource->setPlainText(markdown); m_markdownRenderer->setMarkdown(markdown); }
inline void MainWindow::setPrompt(const QString& prompt) { m_promptBar->setPrompt(prompt); }

inline void MainWindow::showError(const QString& message)
{
    if (!isVisible()) return;
    QMessageBox::critical(this, "错误", message);
}

inline void MainWindow::setBusy(bool busy)
{
    if (m_promptBar) {
        m_promptBar->setButtonsBusy(busy);
    }
}

inline void MainWindow::setCurrentPrompts(const QString& text, const QString& formula, const QString& table)
{
    if (m_promptBar) {
        m_promptBar->setCurrentPrompts(text, formula, table);
    }
}

inline void MainWindow::startAreaSelection(const QImage& fullImage) {
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

inline void MainWindow::updateServiceSelector(const QList<ServiceProfile>& profiles, const QString& currentId)
{
    m_serviceSelector->blockSignals(true);
    m_serviceSelector->clear();

    m_serviceSelector->addItem("全局默认 (远程)", "");

    for (const auto& p : profiles) {
        m_serviceSelector->addItem(p.name, p.id);
    }

    int idx = m_serviceSelector->findData(currentId);
    if (idx >= 0) m_serviceSelector->setCurrentIndex(idx);
    else m_serviceSelector->setCurrentIndex(0);

    m_serviceSelector->blockSignals(false);
}

inline void MainWindow::updateServiceControlButton(const QString& id, bool isRunning)
{
    if (id.isEmpty()) {
        if (m_serviceSelector->currentData().toString().isEmpty()) {
            m_serviceToggleBtn->setText("远程服务");
            m_serviceToggleBtn->setEnabled(false);
            m_serviceToggleBtn->setStyleSheet("");
            return;
        }
    }

    if (m_serviceSelector->currentData().toString() == id) {
        m_serviceToggleBtn->setEnabled(true);
        m_serviceToggleBtn->setText(isRunning ? "已启动" : "未启动");
        m_serviceToggleBtn->setStyleSheet(isRunning ? "background-color: #90EE90; color: black;" : "");
    }
}

inline void MainWindow::updateStopAllAction(int runningCount)
{
    m_stopAllServicesAction->setEnabled(runningCount > 0);
    m_stopAllServicesAction->setText(QString("停止所有服务 (运行中: %1)").arg(runningCount));
}

inline void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) { close(); event->accept(); }
    else QMainWindow::keyPressEvent(event);
}

inline void MainWindow::closeEvent(QCloseEvent *event) { event->ignore(); hide(); }

inline void MainWindow::onPasteImage() {
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

inline void MainWindow::onMarkdownSourceChanged() { m_markdownRenderer->setMarkdown(m_markdownSource->toPlainText()); }
inline void MainWindow::onPromptBarRecognize()
{
    QString currentPrompt = m_promptBar->prompt();
    emit recognizeRequested(currentPrompt, m_imageView->currentBase64());
}
inline void MainWindow::onPromptBarAutoRecognize(const QString& prompt) { emit recognizeRequested(prompt, m_imageView->currentBase64()); }

inline void MainWindow::onManualProcessorTriggered(ContentType type)
{
    QString currentText = currentMarkdownSource();
    if (currentText.isEmpty()) {
        statusBar()->showMessage("内容为空，无法处理。", 3000);
        return;
    }

    emit manualProcessRequested(type);
}

inline void MainWindow::updateScriptCheckboxes()
{
    SettingsManager* s = SettingsManager::instance();

    m_scriptGlobalCheck->blockSignals(true);
    m_scriptTextCheck->blockSignals(true);
    m_scriptFormulaCheck->blockSignals(true);
    m_scriptTableCheck->blockSignals(true);
    m_scriptPureMathCheck->blockSignals(true);
    m_formatterToolCheck->blockSignals(true);

    m_scriptGlobalCheck->setChecked(s->autoExternalProcessBeforeCopy());
    m_scriptTextCheck->setChecked(s->textProcessorEnabled());
    m_scriptFormulaCheck->setChecked(s->formulaProcessorEnabled());
    m_scriptTableCheck->setChecked(s->tableProcessorEnabled());
    m_scriptPureMathCheck->setChecked(s->pureMathProcessorEnabled());
    m_formatterToolCheck->setChecked(s->formatterEnabled());

    m_scriptGlobalCheck->blockSignals(false);
    m_scriptTextCheck->blockSignals(false);
    m_scriptFormulaCheck->blockSignals(false);
    m_scriptTableCheck->blockSignals(false);
    m_scriptPureMathCheck->blockSignals(false);
    m_formatterToolCheck->blockSignals(false);
}

inline QString MainWindow::currentMarkdownSource() const
{
    return m_markdownSource ? m_markdownSource->toPlainText() : QString();
}
inline bool MainWindow::hasImage() const
{
    return m_imageView && m_imageView->hasImage();
}

inline void MainWindow::setRecognizeType(ContentType type)
{
    if (m_copyBar) {
        m_copyBar->setOriginalRecognizeType(type);
    }
}

inline void MainWindow::onCopyCurrentImage()
{
    if (!m_imageView || !m_imageView->hasImage()) {
        return;
    }

    QImage image = m_imageView->currentImage();
    if (!image.isNull()) {
        QApplication::clipboard()->setImage(image);
        statusBar()->showMessage("图片已复制到剪贴板", 3000);
    } else {
        statusBar()->showMessage("无法复制：图片数据无效", 3000);
    }
}

inline void MainWindow::updateCopyImageActionState()
{
    if (m_copyImageAction) {
        bool hasImage = m_imageView && m_imageView->hasImage();
        m_copyImageAction->setEnabled(hasImage);
    }
}


inline void MainWindow::setStreamingMode(bool streaming)
{
    if (!m_markdownRenderer) return;

    if (streaming) {
        m_markdownSource->clear();

        m_markdownRenderer->startStreaming();

        disconnect(m_markdownSource, &QPlainTextEdit::textChanged,
                   this, &MainWindow::onMarkdownSourceChanged);
    } else {
        m_markdownRenderer->stopStreaming();

        disconnect(m_markdownSource, &QPlainTextEdit::textChanged,
                   this, &MainWindow::onMarkdownSourceChanged);
        connect(m_markdownSource, &QPlainTextEdit::textChanged,
                this, &MainWindow::onMarkdownSourceChanged);
    }
}

inline void MainWindow::appendRecognitionResult(const QString& delta)
{
    if (!m_markdownSource) return;

    QTextCursor cursor = m_markdownSource->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(delta);
    m_markdownSource->ensureCursorVisible();

    if (m_markdownRenderer) {
        m_markdownRenderer->appendStreamContent(delta);
    }
}


inline void MainWindow::showHistoryDialog()
{
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle("识别历史记录");
    dialog->resize(900, 600);

    QVBoxLayout* mainLayout = new QVBoxLayout(dialog);

    QListWidget* listWidget = new QListWidget(dialog);
    listWidget->setViewMode(QListView::ListMode);
    listWidget->setIconSize(QSize(250, 250));
    listWidget->setResizeMode(QListView::Adjust);
    listWidget->setAlternatingRowColors(true);
    listWidget->setWordWrap(true);
    listWidget->setStyleSheet("QListWidget::item { padding: 10px; border-bottom: 1px solid #ddd; }");
    mainLayout->addWidget(listWidget);

    QWidget* pageWidget = new QWidget(dialog);
    QHBoxLayout* pageLayout = new QHBoxLayout(pageWidget);
    pageLayout->setContentsMargins(0, 4, 0, 4);

    QPushButton* prevPageBtn = new QPushButton("上一页");
    QPushButton* nextPageBtn = new QPushButton("下一页");
    QLabel* pageInfoLabel = new QLabel();
    pageInfoLabel->setAlignment(Qt::AlignCenter);

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

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Open | QDialogButtonBox::Close, dialog);
    QPushButton* loadBtn = buttons->button(QDialogButtonBox::Open);
    loadBtn->setText("加载到编辑器");

    QPushButton* deleteBtn = buttons->addButton("删除选中", QDialogButtonBox::ActionRole);
    QPushButton* clearBtn = buttons->addButton("清空历史", QDialogButtonBox::ActionRole);

    mainLayout->addWidget(buttons);

    const int pageSize = Constants::HISTORY_PAGE_SIZE;
    int currentPage = 1;
    int totalCount = 0;
    int totalPages = 1;

    std::function<void()> loadCurrentPage;
    loadCurrentPage = [&]() {
        totalCount = HistoryManager::instance()->getTotalCount();
        totalPages = (totalCount == 0) ? 1 : (totalCount + pageSize - 1) / pageSize;

        if (currentPage > totalPages) {
            currentPage = totalPages;
        }
        if (currentPage < 1) {
            currentPage = 1;
        }

        listWidget->clear();

        int offset = (currentPage - 1) * pageSize;

        auto records = HistoryManager::instance()->getRecentRecords(pageSize, offset);
        for (const auto& record : records) {
            QListWidgetItem* item = new QListWidgetItem(listWidget);

            QPixmap pix(record.cachedImagePath);
            if (!pix.isNull()) {
                item->setIcon(QIcon(pix.scaled(250, 250, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
            } else {
                item->setIcon(QIcon::fromTheme("image-missing"));
            }

            QString typeStr = "文字";
            if (record.recognitionType == ContentType::Formula) typeStr = "公式";
            else if (record.recognitionType == ContentType::Table) typeStr = "表格";

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

        prevPageBtn->setEnabled(currentPage > 1);
        nextPageBtn->setEnabled(currentPage < totalPages);

        if (totalCount == 0) {
            pageInfoLabel->setText("暂无记录");
        } else {
            pageInfoLabel->setText(QString("第 %1 / %2 页  (共 %3 条)")
            .arg(currentPage)
            .arg(totalPages)
            .arg(totalCount));
        }

        jumpSpin->setMaximum(totalPages);
        jumpSpin->setValue(currentPage);
        jumpSpin->setEnabled(totalPages > 1);
        jumpBtn->setEnabled(totalPages > 1);
    };

    loadCurrentPage();

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

    connect(jumpSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [&](int value) {
        Q_UNUSED(value);
    });

    connect(loadBtn, &QPushButton::clicked, this, [this, listWidget, dialog](){
        auto currentItem = listWidget->currentItem();
        if (currentItem) {
            int recordId = currentItem->data(Qt::UserRole).toInt();
            emit loadHistoryRecordRequested(recordId);
            dialog->close();
        }
    });

    connect(deleteBtn, &QPushButton::clicked, this, [&]() {
        auto currentItem = listWidget->currentItem();
        if (currentItem) {
            int recordId = currentItem->data(Qt::UserRole).toInt();
            HistoryManager::instance()->deleteRecord(recordId);
            loadCurrentPage();
        }
    });

    connect(clearBtn, &QPushButton::clicked, this, [&]() {
        if (QMessageBox::question(dialog, "确认", "确定要清空所有历史记录吗？") == QMessageBox::Yes) {
            HistoryManager::instance()->clearAll();
            currentPage = 1;
            loadCurrentPage();
        }
    });

    connect(listWidget, &QListWidget::itemDoubleClicked, this, [this, listWidget, dialog](QListWidgetItem* item){
        int recordId = item->data(Qt::UserRole).toInt();
        emit loadHistoryRecordRequested(recordId);
        dialog->close();
    });

    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::close);

    dialog->exec();
    dialog->deleteLater();
}

inline QImage MainWindow::currentImage() const
{
    if (m_imageView) {
        return m_imageView->currentImage();
    }
    return QImage();
}

inline void MainWindow::setCurrentServiceName(const QString& name)
{
    if (m_markdownRenderer) {
        m_markdownRenderer->setServiceName(name);
    }

    if (m_copyBar) {
        m_copyBar->setCurrentServiceName(name);
    }
}

#endif // MAINWINDOW_IMPL_H
