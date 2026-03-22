#include "mainwindow.h"
#include "imageviewwidget.h"
#include "markdownrenderer.h"
#include "markdownsourceeditor.h"
#include "promptbar.h"
#include "networkmanager.h"
#include "screenshotmanager.h"
#include "globalshortcutmanager.h"

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
#include <QImage>
#include <QPixmap>
#include <QTimer>
#include <QSettings>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QScreen>
#include <QGuiApplication>
#include <QKeySequence>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QStyle>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    m_networkManager = new NetworkManager(this);
    connect(m_networkManager, &NetworkManager::requestFinished,
            this, &MainWindow::onRequestFinished);

    m_autoRecognizeDebounceTimer = new QTimer(this);
    m_autoRecognizeDebounceTimer->setSingleShot(true);
    connect(m_autoRecognizeDebounceTimer, &QTimer::timeout, this, &MainWindow::onImageChanged);

    m_screenshotManager = new ScreenshotManager(this);

    // 修改：截图成功后，设置图片并显示窗口
    connect(m_screenshotManager, &ScreenshotManager::screenshotCaptured,
            this, [this](const QImage& image) {
                m_imageView->setImage(image);
                // 截图完成后显示窗口
                show();
                raise();
                activateWindow();
            });

    connect(m_screenshotManager, &ScreenshotManager::screenshotFailed,
            this, [this](const QString& error) {
                // 仅在窗口可见时显示错误框，避免截图过程中突然弹框干扰
                if (isVisible()) {
                    QMessageBox::warning(this, "截图失败", error);
                } else {
                    qDebug() << "Screenshot failed:" << error;
                }
            });

    setupUi();
    setupMenuBar();
    setupGlobalShortcuts();
    createTrayIcon();

    QSettings settings;
    m_autoUseLastPrompt = settings.value("auto_use_last_prompt", true).toBool();
    m_lastUsedPrompt = "文字识别:";
}

MainWindow::~MainWindow()
{
}

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

    connect(m_imageView, &ImageViewWidget::imageChanged, this, [this]() {
        m_autoRecognizeDebounceTimer->start(300);
    });

    m_markdownRenderer = new MarkdownRenderer(this);
    leftSplitter->addWidget(m_markdownRenderer);

    leftSplitter->setSizes({400, 400});
    leftSplitter->setStretchFactor(0, 1);
    leftSplitter->setStretchFactor(1, 1);

    QSplitter* rightSplitter = new QSplitter(Qt::Vertical);
    mainSplitter->addWidget(rightSplitter);

    m_promptBar = new PromptBar(this);
    rightSplitter->addWidget(m_promptBar);

    m_markdownSource = new MarkdownSourceEditor(this);
    rightSplitter->addWidget(m_markdownSource);

    rightSplitter->setStretchFactor(0, 0);
    rightSplitter->setStretchFactor(1, 1);
    rightSplitter->setSizes({m_promptBar->minimumSizeHint().height(), 500});

    mainSplitter->setSizes({700, 500});

    connect(m_promptBar, &PromptBar::recognizeRequested, this, &MainWindow::onRecognize);
    connect(m_promptBar, &PromptBar::autoRecognizeRequested, this, &MainWindow::onAutoRecognize);
    connect(m_markdownSource, &QPlainTextEdit::textChanged, this, &MainWindow::onMarkdownSourceChanged);

    m_pasteShortcut = new QShortcut(QKeySequence::Paste, this);
    m_pasteShortcut->setContext(Qt::ApplicationShortcut);
    connect(m_pasteShortcut, &QShortcut::activated, this, &MainWindow::pasteImage);
}

void MainWindow::setupMenuBar()
{
    QMenuBar* menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    QMenu* fileMenu = menuBar->addMenu("文件");
    QAction* openAction = fileMenu->addAction("打开");
    connect(openAction, &QAction::triggered, this, &MainWindow::openImageFile);
    QAction* saveAction = fileMenu->addAction("保存");
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveMarkdownAsFile);

    QAction* screenshotAction = fileMenu->addAction("截图");
    connect(screenshotAction, &QAction::triggered, this, &MainWindow::takeScreenshot);

    QAction* quitAction = fileMenu->addAction("退出");
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    QMenu* settingsMenu = menuBar->addMenu("设置");
    QAction* setUrlAction = settingsMenu->addAction("设置服务器地址");
    connect(setUrlAction, &QAction::triggered, this, [this]() {
        bool ok;
        QString newUrl = QInputDialog::getText(this, "设置服务器地址",
                                               "请输入 OpenAI 兼容的 API 端点:",
                                               QLineEdit::Normal,
                                               m_networkManager->serverUrl(),
                                               &ok);
        if (ok && !newUrl.isEmpty()) {
            m_networkManager->setServerUrl(newUrl);
        }
    });

    QAction* setScreenshotShortcutAction = settingsMenu->addAction("设置截图快捷键");
    connect(setScreenshotShortcutAction, &QAction::triggered, this, &MainWindow::onSetScreenshotShortcut);

    QAction* autoUseLastPromptAction = settingsMenu->addAction("图片变更时自动使用上次识别类型");
    autoUseLastPromptAction->setCheckable(true);
    autoUseLastPromptAction->setChecked(m_autoUseLastPrompt);
    connect(autoUseLastPromptAction, &QAction::toggled, this, &MainWindow::onAutoUseLastPromptToggled);
}

void MainWindow::createTrayIcon()
{
    m_trayIcon = new QSystemTrayIcon(this);
    QIcon icon = QIcon::fromTheme("hiocr", QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));
    m_trayIcon->setIcon(icon);
    m_trayIcon->setToolTip("hiocr");

    QMenu* trayMenu = new QMenu(this);

    QAction* showAction = trayMenu->addAction("显示窗口");
    connect(showAction, &QAction::triggered, this, [this]() {
        show();
        raise();
        activateWindow();
    });

    trayMenu->addSeparator();

    // 新增：具体的识别模式菜单项
    QAction* textAction = trayMenu->addAction("文字识别");
    connect(textAction, &QAction::triggered, this, &MainWindow::onTrayTextRecognize);

    QAction* formulaAction = trayMenu->addAction("公式识别");
    connect(formulaAction, &QAction::triggered, this, &MainWindow::onTrayFormulaRecognize);

    QAction* tableAction = trayMenu->addAction("表格识别");
    connect(tableAction, &QAction::triggered, this, &MainWindow::onTrayTableRecognize);

    trayMenu->addSeparator();

    QAction* quitAction = trayMenu->addAction("退出");
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    m_trayIcon->setContextMenu(trayMenu);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayIconActivated);
    m_trayIcon->show();
}

// 托盘图标点击事件：左键点击直接开始截图
void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
        case QSystemTrayIcon::Trigger:
        case QSystemTrayIcon::DoubleClick:
            // 左键/双击：开始截图（使用默认或上次的提示词）
            takeScreenshot();
            break;
        default:
            break;
    }
}

// 托盘菜单：文字识别
void MainWindow::onTrayTextRecognize()
{
    m_tempPromptOverride = "文字识别:";
    takeScreenshot();
}

// 托盘菜单：公式识别
void MainWindow::onTrayFormulaRecognize()
{
    m_tempPromptOverride = "公式识别:";
    takeScreenshot();
}

// 托盘菜单：表格识别
void MainWindow::onTrayTableRecognize()
{
    m_tempPromptOverride = "表格识别:";
    takeScreenshot();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    event->ignore();
    hide();
    static bool firstTime = true;
    if (firstTime) {
        m_trayIcon->showMessage("hiocr", "程序已最小化到系统托盘，点击图标可截图。");
        firstTime = false;
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        close();
        return;
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::onRecognize()
{
    performRecognition(m_promptBar->prompt());
}

void MainWindow::onAutoRecognize(const QString& prompt)
{
    performRecognition(prompt);
}

void MainWindow::performRecognition(const QString& prompt)
{
    QString currentBase64 = m_imageView->currentBase64();
    if (currentBase64.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择图片");
        return;
    }

    m_lastUsedPrompt = prompt;
    m_promptBar->setButtonsBusy(true);
    m_networkManager->sendRequest(currentBase64, prompt);
}

void MainWindow::onRequestFinished(const QString& result, bool success, const QString& error)
{
    m_promptBar->setButtonsBusy(false);

    if (!success) {
        QMessageBox::critical(this, "请求失败", error);
        return;
    }

    m_markdownSource->setPlainText(result);
    m_markdownRenderer->setMarkdown(result);
}

void MainWindow::onMarkdownSourceChanged()
{
    QString currentMarkdown = m_markdownSource->toPlainText();
    m_markdownRenderer->setMarkdown(currentMarkdown);
}

void MainWindow::pasteImage()
{
    QWidget* focusWidget = QApplication::focusWidget();

    if (qobject_cast<QLineEdit*>(focusWidget) ||
        qobject_cast<QPlainTextEdit*>(focusWidget) ||
        qobject_cast<QTextEdit*>(focusWidget)) {
        if (auto* lineEdit = qobject_cast<QLineEdit*>(focusWidget)) {
            lineEdit->paste();
        } else if (auto* plainTextEdit = qobject_cast<QPlainTextEdit*>(focusWidget)) {
            plainTextEdit->paste();
        } else if (auto* textEdit = qobject_cast<QTextEdit*>(focusWidget)) {
            textEdit->paste();
        }
        return;
        }

        QClipboard* clipboard = QApplication::clipboard();
        const QMimeData* mimeData = clipboard->mimeData();

        if (mimeData->hasImage()) {
            QImage image = clipboard->image();
            if (image.isNull()) {
                QPixmap pixmap = clipboard->pixmap();
                if (!pixmap.isNull()) {
                    image = pixmap.toImage();
                }
            }
            if (!image.isNull()) {
                m_imageView->setImage(image);
            } else {
                QMessageBox::warning(this, "粘贴失败", "剪贴板中的图片无效");
            }
        } else {
            QMessageBox::warning(this, "粘贴失败", "剪贴板中不包含图片数据");
        }
}

void MainWindow::onAutoUseLastPromptToggled(bool checked)
{
    m_autoUseLastPrompt = checked;
    QSettings settings;
    settings.setValue("auto_use_last_prompt", checked);
}

void MainWindow::onImageChanged()
{
    if (!m_imageView->hasImage())
        return;

    QString promptToUse;

    // 优先使用托盘菜单设置的临时提示词
    if (!m_tempPromptOverride.isEmpty()) {
        promptToUse = m_tempPromptOverride;
        m_tempPromptOverride.clear(); // 用完即销
    } else {
        // 否则使用设置逻辑
        promptToUse = m_autoUseLastPrompt ? m_lastUsedPrompt : "文字识别:";
    }

    // 同步更新 PromptBar 显示
    m_promptBar->setPrompt(promptToUse);

    performRecognition(promptToUse);
}

void MainWindow::openImageFile()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("选择图片文件"),
                                                    QString(),
                                                    tr("Images (*.png *.jpg *.jpeg *.bmp *.gif)"));
    if (!fileName.isEmpty()) {
        m_imageView->setImage(fileName);
    }
}

void MainWindow::saveMarkdownAsFile()
{
    QString content = m_markdownSource->toPlainText();
    if (content.isEmpty()) {
        QMessageBox::information(this, "保存", "没有可保存的Markdown内容");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("保存Markdown文件"),
                                                    QString(),
                                                    tr("Markdown Files (*.md)"));
    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "保存失败", "无法创建文件: " + fileName);
        return;
    }

    QTextStream out(&file);
    out << content;
    file.close();

    QMessageBox::information(this, "保存成功", "文件已保存至: " + fileName);
}

void MainWindow::takeScreenshot()
{
    m_screenshotManager->takeScreenshot();
}

void MainWindow::onSetScreenshotShortcut()
{
    QSettings settings;
    bool ok;
    QString text = QInputDialog::getText(this, "设置截图快捷键",
                                         "请输入组合键（如 Ctrl+Shift+S）:",
                                         QLineEdit::Normal,
                                         settings.value("shortcuts/screenshot").toString(),
                                         &ok);
    if (!ok || text.isEmpty()) return;

    QKeySequence seq(text);
    if (seq.isEmpty()) {
        QMessageBox::warning(this, "无效快捷键", "输入的快捷键无效，请重新输入。");
        return;
    }

    settings.setValue("shortcuts/screenshot", seq.toString());

    if (QGuiApplication::platformName() == "wayland") {
        QMessageBox::information(this, "提示", "快捷键已保存。在 Wayland 环境下，修改全局快捷键可能需要重启应用或查看系统设置。");
    }

    setupGlobalShortcuts();
}

void MainWindow::loadImageFromFile(const QString& path)
{
    m_imageView->setImage(path);
}

void MainWindow::setupGlobalShortcuts()
{
    QSettings settings;
    QString key = settings.value("shortcuts/screenshot", "Ctrl+Shift+S").toString();
    m_screenshotShortcutKey = key;
    QKeySequence seq(key);

    if (m_screenshotShortcut) {
        delete m_screenshotShortcut;
        m_screenshotShortcut = nullptr;
    }

    if (!seq.isEmpty()) {
        m_screenshotShortcut = new QShortcut(seq, this);
        m_screenshotShortcut->setContext(Qt::ApplicationShortcut);
        connect(m_screenshotShortcut, &QShortcut::activated, this, &MainWindow::takeScreenshot);
    }

    if (QGuiApplication::platformName() == "wayland") {
        GlobalShortcutManager* gsm = GlobalShortcutManager::instance();
        disconnect(gsm, nullptr, this, nullptr);
        connect(gsm, &GlobalShortcutManager::activated, this, &MainWindow::onGlobalShortcutActivated, Qt::UniqueConnection);
        gsm->registerShortcut("screenshot", "Take Screenshot", key);
        gsm->startListening();
    }
}

void MainWindow::onGlobalShortcutActivated(const QString& id)
{
    if (id == "screenshot") {
        show();
        raise();
        activateWindow();
        takeScreenshot();
    }
}
