#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QObject>
#include <QImage>
#include <QClipboard>

class MainWindow;
class ScreenshotManager;
class RecognitionManager;
class SettingsManager;
class TrayManager;
class ShortcutHandler;

class AppController : public QObject
{
    Q_OBJECT

public:
    explicit AppController(QObject *parent = nullptr);
    ~AppController();

    // 初始化控制器，建立所有连接
    void initialize();

    // 获取主窗口实例（用于 main.cpp 显示）
    MainWindow* mainWindow() const;

public slots:
    // --- 动作触发入口 ---
    void onRecognitionFinished(const QString& markdown);
    void takeScreenshot();             // 通用截图
    void takeTextRecognizeScreenshot(); // 文字识别截图
    void takeFormulaRecognizeScreenshot(); // 公式识别截图
    void takeTableRecognizeScreenshot(); // 表格识别截图
    void showWindow();
    void quitApp();

signals:
    // --- 数据更新信号（通知 UI） ---
    void imageChanged(const QImage& image);
    void recognitionResultReady(const QString& markdown);
    void recognitionFailed(const QString& error);
    void busyStateChanged(bool busy);

    // 内部信号，用于协调异步流程
    void requestAreaSelection(const QImage& fullImage);

private slots:
    // --- 内部逻辑处理 ---
    void onScreenshotCaptured(const QImage& image);
    void onScreenshotFailed(const QString& error);
    void onAreaSelected(const QRect& rect);
    void onSettingsChanged();

private:
    void setupManagers();
    void setupConnections();
    void applySettings();

    // Manager 实例
    MainWindow* m_mainWindow = nullptr;
    ScreenshotManager* m_screenshotManager = nullptr;
    RecognitionManager* m_recognitionManager = nullptr;
    SettingsManager* m_settings = nullptr;
    TrayManager* m_trayManager = nullptr;
    ShortcutHandler* m_shortcutHandler = nullptr;

    // 临时状态
    QImage m_pendingFullScreenshot; // 等待选区的全屏截图
    QString m_pendingPromptOverride; // 选区完成后要使用的提示词
};

#endif // APPCONTROLLER_H
