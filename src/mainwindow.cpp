#include "mainwindow.h"
#include "imageviewwidget.h"
#include "markdownrenderer.h"
#include "markdownsourceeditor.h"
#include "promptbar.h"
#include "networkmanager.h"
#include "screenshotmanager.h"
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

MainWindow::MainWindow(QWidget* parent)
: QMainWindow(parent)
, m_screenshotShortcut(nullptr)
, m_textShortcut(nullptr)
, m_formulaShortcut(nullptr)
, m_tableShortcut(nullptr)
{
    m_networkManager = new NetworkManager(this);
    connect(m_networkManager, &NetworkManager::requestFinished,
            this, &MainWindow::onRequestFinished);

    m_autoRecognizeDebounceTimer = new QTimer(this);
    m_autoRecognizeDebounceTimer->setSingleShot(true);
    connect(m_autoRecognizeDebounceTimer, &QTimer::timeout, this, &MainWindow::onImageChanged);

    m_screenshotManager = new ScreenshotManager(this);
    // 截图成功后：加载图片，延迟显示窗口（避免 Wayland 协议错误）
    connect(m_screenshotManager, &ScreenshotManager::screenshotCaptured,
            this, [this](const QImage& image) {
                m_imageView->setImage(image);
                // 延迟显示，让 Wayland 有机会发送 configure 事件
                QTimer::singleShot(0, this, [this]() {
                    if (!isVisible()) {
                        show();
                    }
                    raise();
                    activateWindow();
                });
            });
    connect(m_screenshotManager, &ScreenshotManager::screenshotFailed,
            this, [this](const QString& error) {
                if (isVisible())
                    QMessageBox::warning(this, "截图失败", error);
                else
                    qDebug() << "Screenshot failed:" << error;
            });

    setupUi();
    setupMenuBar();
    setupGlobalShortcuts();
    createTrayIcon();

    QSettings settings;
    m_autoUseLastPrompt = settings.value("auto_use_last_prompt", true).toBool();
    m_lastUsedPrompt = "文字识别:";
}

MainWindow::~MainWindow() {}

// ---------- UI 初始化 ----------
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
        if (ok && !newUrl.isEmpty())
            m_networkManager->setServerUrl(newUrl);
    });

    QAction* setScreenshotShortcutAction = settingsMenu->addAction("设置截图快捷键");
    connect(setScreenshotShortcutAction, &QAction::triggered, this, &MainWindow::onSetScreenshotShortcut);
    QAction* setTextShortcutAction = settingsMenu->addAction("设置文字识别快捷键");
    connect(setTextShortcutAction, &QAction::triggered, this, &MainWindow::onSetTextRecognizeShortcut);
    QAction* setFormulaShortcutAction = settingsMenu->addAction("设置公式识别快捷键");
    connect(setFormulaShortcutAction, &QAction::triggered, this, &MainWindow::onSetFormulaRecognizeShortcut);
    QAction* setTableShortcutAction = settingsMenu->addAction("设置表格识别快捷键");
    connect(setTableShortcutAction, &QAction::triggered, this, &MainWindow::onSetTableRecognizeShortcut);

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

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
        takeScreenshot();
}

// ========== 托盘菜单槽函数（直接调用快速识别） ==========
void MainWindow::onTrayTextRecognize()
{
    onQuickTextRecognize();
}

void MainWindow::onTrayFormulaRecognize()
{
    onQuickFormulaRecognize();
}

void MainWindow::onTrayTableRecognize()
{
    onQuickTableRecognize();
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
    if (event->key() == Qt::Key_Escape) close();
    else QMainWindow::keyPressEvent(event);
}

void MainWindow::onRecognize() { performRecognition(m_promptBar->prompt()); }
void MainWindow::onAutoRecognize(const QString& prompt) { performRecognition(prompt); }

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
    m_markdownRenderer->setMarkdown(m_markdownSource->toPlainText());
}

void MainWindow::pasteImage()
{
    QWidget* focusWidget = QApplication::focusWidget();
    if (qobject_cast<QLineEdit*>(focusWidget) ||
        qobject_cast<QPlainTextEdit*>(focusWidget) ||
        qobject_cast<QTextEdit*>(focusWidget)) {
        if (auto* lineEdit = qobject_cast<QLineEdit*>(focusWidget))
            lineEdit->paste();
        else if (auto* plainTextEdit = qobject_cast<QPlainTextEdit*>(focusWidget))
            plainTextEdit->paste();
        else if (auto* textEdit = qobject_cast<QTextEdit*>(focusWidget))
            textEdit->paste();
        return;
        }

        QClipboard* clipboard = QApplication::clipboard();
        const QMimeData* mimeData = clipboard->mimeData();
        if (mimeData->hasImage()) {
            QImage image = clipboard->image();
            if (image.isNull()) {
                QPixmap pixmap = clipboard->pixmap();
                if (!pixmap.isNull()) image = pixmap.toImage();
            }
            if (!image.isNull())
                m_imageView->setImage(image);
            else
                QMessageBox::warning(this, "粘贴失败", "剪贴板中的图片无效");
        } else {
            QMessageBox::warning(this, "粘贴失败", "剪贴板中不包含图片数据");
        }
}

void MainWindow::onAutoUseLastPromptToggled(bool checked)
{
    m_autoUseLastPrompt = checked;
    QSettings().setValue("auto_use_last_prompt", checked);
}

void MainWindow::onImageChanged()
{
    if (!m_imageView->hasImage()) return;

    QString promptToUse;
    if (!m_tempPromptOverride.isEmpty()) {
        promptToUse = m_tempPromptOverride;
        m_tempPromptOverride.clear();
    } else {
        promptToUse = m_autoUseLastPrompt ? m_lastUsedPrompt : "文字识别:";
    }
    m_promptBar->setPrompt(promptToUse);
    performRecognition(promptToUse);
}

void MainWindow::openImageFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("选择图片文件"), QString(),
                                                    tr("Images (*.png *.jpg *.jpeg *.bmp *.gif)"));
    if (!fileName.isEmpty()) m_imageView->setImage(fileName);
}

void MainWindow::saveMarkdownAsFile()
{
    QString content = m_markdownSource->toPlainText();
    if (content.isEmpty()) {
        QMessageBox::information(this, "保存", "没有可保存的Markdown内容");
        return;
    }
    QString fileName = QFileDialog::getSaveFileName(this, tr("保存Markdown文件"), QString(),
                                                    tr("Markdown Files (*.md)"));
    if (fileName.isEmpty()) return;
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

void MainWindow::takeScreenshot() { m_screenshotManager->takeScreenshot(); }

void MainWindow::setShortcutForKey(const QString& key, QString& currentKey)
{
    QSettings settings;
    bool ok;
    QString text = QInputDialog::getText(this, "设置快捷键",
                                         QString("请输入 %1 的组合键（如 Ctrl+Shift+S）:").arg(key),
                                         QLineEdit::Normal,
                                         currentKey,
                                         &ok);
    if (!ok || text.isEmpty()) return;

    QKeySequence seq(text);
    if (seq.isEmpty()) {
        QMessageBox::warning(this, "无效快捷键", "输入的快捷键无效，请重新输入。");
        return;
    }

    settings.setValue("shortcuts/" + key, seq.toString());
    currentKey = seq.toString();
    setupGlobalShortcuts();
    QMessageBox::information(this, "设置成功", "快捷键已更新。请重新启动程序使设置完全生效（某些环境下需要重启）。");
}

void MainWindow::onSetScreenshotShortcut() { setShortcutForKey("screenshot", m_screenshotShortcutKey); }
void MainWindow::onSetTextRecognizeShortcut() { setShortcutForKey("text_recognize", m_textRecognizeShortcutKey); }
void MainWindow::onSetFormulaRecognizeShortcut() { setShortcutForKey("formula_recognize", m_formulaRecognizeShortcutKey); }
void MainWindow::onSetTableRecognizeShortcut() { setShortcutForKey("table_recognize", m_tableRecognizeShortcutKey); }

// 统一行为：总是先截图，再使用对应提示词识别
void MainWindow::onQuickTextRecognize()
{
    m_tempPromptOverride = "文字识别:";
    takeScreenshot();
}
void MainWindow::onQuickFormulaRecognize()
{
    m_tempPromptOverride = "公式识别:";
    takeScreenshot();
}
void MainWindow::onQuickTableRecognize()
{
    m_tempPromptOverride = "表格识别:";
    takeScreenshot();
}

void MainWindow::setupGlobalShortcuts()
{
    QSettings settings;
    m_screenshotShortcutKey = settings.value("shortcuts/screenshot", "Ctrl+Shift+S").toString();
    m_textRecognizeShortcutKey = settings.value("shortcuts/text_recognize", "Ctrl+Shift+T").toString();
    m_formulaRecognizeShortcutKey = settings.value("shortcuts/formula_recognize", "Ctrl+Shift+F").toString();
    m_tableRecognizeShortcutKey = settings.value("shortcuts/table_recognize", "Ctrl+Shift+B").toString();

    if (m_screenshotShortcut) delete m_screenshotShortcut;
    if (m_textShortcut) delete m_textShortcut;
    if (m_formulaShortcut) delete m_formulaShortcut;
    if (m_tableShortcut) delete m_tableShortcut;

    auto createLocalShortcut = [this](const QString& key, QShortcut*& shortcut, std::function<void()> slot) {
        if (!key.isEmpty()) {
            shortcut = new QShortcut(QKeySequence(key), this);
            shortcut->setContext(Qt::ApplicationShortcut);
            connect(shortcut, &QShortcut::activated, this, slot);
        }
    };
    createLocalShortcut(m_screenshotShortcutKey, m_screenshotShortcut, [this]() { takeScreenshot(); });
    createLocalShortcut(m_textRecognizeShortcutKey, m_textShortcut, [this]() { onQuickTextRecognize(); });
    createLocalShortcut(m_formulaRecognizeShortcutKey, m_formulaShortcut, [this]() { onQuickFormulaRecognize(); });
    createLocalShortcut(m_tableRecognizeShortcutKey, m_tableShortcut, [this]() { onQuickTableRecognize(); });

    if (QGuiApplication::platformName() == "wayland") {
        bool isKde = qgetenv("XDG_CURRENT_DESKTOP").toLower().contains("kde");
        #ifdef HAVE_KF6
        if (isKde) {
            KdeGlobalShortcut* kss = KdeGlobalShortcut::instance();
            disconnect(kss, nullptr, this, nullptr);
            connect(kss, &KdeGlobalShortcut::activated, this, &MainWindow::onGlobalShortcutActivated);
            kss->registerShortcut("screenshot", "Take Screenshot", m_screenshotShortcutKey);
            kss->registerShortcut("text_recognize", "Text Recognize", m_textRecognizeShortcutKey);
            kss->registerShortcut("formula_recognize", "Formula Recognize", m_formulaRecognizeShortcutKey);
            kss->registerShortcut("table_recognize", "Table Recognize", m_tableRecognizeShortcutKey);
            kss->startListening();
            qDebug() << "Using KGlobalAccel for global shortcuts";
            return;
        }
        #endif
        GlobalShortcutManager* gsm = GlobalShortcutManager::instance();
        disconnect(gsm, nullptr, this, nullptr);
        connect(gsm, &GlobalShortcutManager::activated, this, &MainWindow::onGlobalShortcutActivated, Qt::UniqueConnection);
        connect(gsm, &GlobalShortcutManager::errorOccurred, this, [this](const QString& msg) {
            qWarning() << "GlobalShortcut error:" << msg;
        });
        gsm->registerShortcut("screenshot", "Take Screenshot", m_screenshotShortcutKey);
        gsm->registerShortcut("text_recognize", "Text Recognize", m_textRecognizeShortcutKey);
        gsm->registerShortcut("formula_recognize", "Formula Recognize", m_formulaRecognizeShortcutKey);
        gsm->registerShortcut("table_recognize", "Table Recognize", m_tableRecognizeShortcutKey);
        gsm->startListening();
        qDebug() << "Using xdg-desktop-portal for global shortcuts";
    } else {
        qDebug() << "X11: using local QShortcut (global shortcuts not fully supported)";
    }
}

void MainWindow::onGlobalShortcutActivated(const QString& id)
{
    if (id == "screenshot") {
        takeScreenshot();
    } else if (id == "text_recognize") {
        onQuickTextRecognize();
    } else if (id == "formula_recognize") {
        onQuickFormulaRecognize();
    } else if (id == "table_recognize") {
        onQuickTableRecognize();
    }
}

void MainWindow::loadImageFromFile(const QString& path)
{
    m_imageView->setImage(path);
}
