#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QShortcut>

class ImageViewWidget;
class MarkdownRenderer;
class MarkdownSourceEditor;
class PromptBar;
class ScreenshotManager;
class SettingsManager;
class TrayManager;
class RecognitionManager;

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
    // UI 响应槽
    void onPasteImage();
    void onMarkdownSourceChanged();

    // 业务逻辑槽（由 Manager 触发）
    void onRecognitionFinished(const QString& result);
    void onRecognitionFailed(const QString& error);
    void onBusyChanged(bool busy);

    // 托盘与快捷键动作
    void onActionScreenshot();
    void onActionTextRecognize();
    void onActionFormulaRecognize();
    void onActionTableRecognize();
    void onActionShowWindow();

    // 设置变更响应
    void onShortcutSettingChanged();

private:
    void setupUi();
    void setupConnections();
    void setupMenuBar();
    void initManagers(); // 初始化各个管理器
    void applyGlobalShortcuts(); // 应用全局快捷键（读取设置）

    // UI 组件
    ImageViewWidget* m_imageView;
    MarkdownRenderer* m_markdownRenderer;
    MarkdownSourceEditor* m_markdownSource;
    PromptBar* m_promptBar;
    QShortcut* m_pasteShortcut;

    // 管理器
    ScreenshotManager* m_screenshotManager = nullptr;
    SettingsManager* m_settings = nullptr;
    TrayManager* m_trayManager = nullptr;
    RecognitionManager* m_recognitionManager = nullptr;

    // 本地快捷键对象（用于 X11 或内部快捷键）
    QShortcut* m_shortcutScreenshot = nullptr;
    QShortcut* m_shortcutText = nullptr;
    QShortcut* m_shortcutFormula = nullptr;
    QShortcut* m_shortcutTable = nullptr;
};

#endif // MAINWINDOW_H
