#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H
#include "servicemanager.h"

#include <QObject>
#include <QImage>
#include <QClipboard>
#include <QLocalServer> // 【新增】单实例通信服务端

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

    // 【新增】处理外部传入的命令行参数
    void handleCommandLineArguments(const QString& imagePath, const QString& resultText);

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

    // 【新增】单实例通信槽函数
    void onNewInstanceConnected();

private:
    void setupManagers();
    void setupConnections();
    void applySettings();
    void setupSingleInstance(); // 【新增】设置单实例监听

    // Manager 实例
    MainWindow* m_mainWindow = nullptr;
    ScreenshotManager* m_screenshotManager = nullptr;
    RecognitionManager* m_recognitionManager = nullptr;
    SettingsManager* m_settings = nullptr;
    TrayManager* m_trayManager = nullptr;
    ShortcutHandler* m_shortcutHandler = nullptr;
    ServiceManager* m_serviceManager = nullptr;

    QLocalServer* m_localServer = nullptr; // 【新增】

    // 临时状态
    QImage m_pendingFullScreenshot;
    QString m_pendingPromptOverride;

    // 用于暂存识别请求（当服务正在启动时）
    QString m_pendingPrompt;
    QString m_pendingBase64;
    bool m_retryAfterServiceStart = false;
};

#endif // APPCONTROLLER_H
