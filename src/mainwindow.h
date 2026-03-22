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
    void onSetTextRecognizeShortcut();
    void onSetFormulaRecognizeShortcut();
    void onSetTableRecognizeShortcut();

    void onGlobalShortcutActivated(const QString& id);
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);

    void onTrayTextRecognize();
    void onTrayFormulaRecognize();
    void onTrayTableRecognize();

    void onQuickTextRecognize();
    void onQuickFormulaRecognize();
    void onQuickTableRecognize();

private:
    void setupUi();
    void setupMenuBar();
    void performRecognition(const QString& prompt);
    void setupGlobalShortcuts();
    void createTrayIcon();
    void setShortcutForKey(const QString& key, QString& currentKey);

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
    QShortcut* m_textShortcut;
    QShortcut* m_formulaShortcut;
    QShortcut* m_tableShortcut;

    QString m_screenshotShortcutKey;
    QString m_textRecognizeShortcutKey;
    QString m_formulaRecognizeShortcutKey;
    QString m_tableRecognizeShortcutKey;

    ScreenshotManager* m_screenshotManager;
    QSystemTrayIcon* m_trayIcon;

    QString m_tempPromptOverride;
};

#endif // MAINWINDOW_H
