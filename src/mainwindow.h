#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QShortcut>
#include <QSystemTrayIcon>

class ImageViewWidget;
class MarkdownRenderer;
class MarkdownSourceEditor;
class PromptBar;
class NetworkManager;
class ScreenshotManager;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    void loadImageFromFile(const QString& path);

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onRecognize();
    void onAutoRecognize(const QString& prompt);
    void onRequestFinished(const QString& result, bool success, const QString& error);
    void onMarkdownSourceChanged();
    void pasteImage();
    void onImageChanged();
    void onAutoUseLastPromptToggled(bool checked);
    void openImageFile();
    void saveMarkdownAsFile();

    void takeScreenshot();
    void onSetScreenshotShortcut();

    void onGlobalShortcutActivated(const QString& id);
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);

    // 新增：托盘菜单特定的截图槽函数
    void onTrayTextRecognize();
    void onTrayFormulaRecognize();
    void onTrayTableRecognize();

private:
    void setupUi();
    void setupMenuBar();
    void performRecognition(const QString& prompt);
    void setupGlobalShortcuts();
    void createTrayIcon();

    ImageViewWidget* m_imageView;
    MarkdownRenderer* m_markdownRenderer;
    MarkdownSourceEditor* m_markdownSource;
    PromptBar* m_promptBar;
    NetworkManager* m_networkManager;
    QShortcut* m_pasteShortcut;

    QString m_lastUsedPrompt;
    bool m_autoUseLastPrompt;
    QTimer* m_autoRecognizeDebounceTimer;

    QShortcut* m_screenshotShortcut;
    QString m_screenshotShortcutKey;

    ScreenshotManager* m_screenshotManager;
    QSystemTrayIcon* m_trayIcon;

    // 新增：用于临时覆盖下一次识别的提示词
    QString m_tempPromptOverride;
};

#endif // MAINWINDOW_H
