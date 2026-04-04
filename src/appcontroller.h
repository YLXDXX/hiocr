#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include "servicemanager.h"
#include "copyprocessor.h" // 引入
#include <QObject>
#include <QImage>

class MainWindow;
class ScreenshotManager;
class RecognitionManager;
class SettingsManager;
class TrayManager;
class ShortcutHandler;
class SingleApplication;

class AppController : public QObject
{
    Q_OBJECT

public:
    explicit AppController(QObject *parent = nullptr);
    ~AppController();

    void initialize();
    MainWindow* mainWindow() const;
    void handleCommandLineArguments(const QString& imagePath, const QString& resultText);

public slots:
    void onRecognitionFinished(const QString& markdown);
    void takeScreenshot();
    void takeTextRecognizeScreenshot();
    void takeFormulaRecognizeScreenshot();
    void takeTableRecognizeScreenshot();
    void showWindow();
    void quitApp();

signals:
    void imageChanged(const QImage& image);
    void recognitionResultReady(const QString& markdown);
    void recognitionFailed(const QString& error);
    void busyStateChanged(bool busy);
    void requestAreaSelection(const QImage& fullImage);

private slots:
    void onScreenshotCaptured(const QImage& image);
    void onScreenshotFailed(const QString& error);
    void onAreaSelected(const QRect& rect);
    void onSettingsChanged();

    // 服务管理相关槽函数
    void onServiceSelected(const QString& id);
    void toggleService(const QString& id);

    void onSingleInstanceMessageReceived(const QByteArray& message);

    // 【新增】处理手动触发脚本
    void onManualProcessorTriggered(ContentType type);

private:
    void setupManagers();
    void setupConnections();
    void applySettings();
    void setupSingleInstance();

    // 应用服务配置
    void applyServiceConfig(const QString& serviceId);

    MainWindow* m_mainWindow = nullptr;
    ScreenshotManager* m_screenshotManager = nullptr;
    RecognitionManager* m_recognitionManager = nullptr;
    SettingsManager* m_settings = nullptr;
    TrayManager* m_trayManager = nullptr;
    ShortcutHandler* m_shortcutHandler = nullptr;
    ServiceManager* m_serviceManager = nullptr;

    // 【新增】统一管理 CopyProcessor，避免分散在 UI 中
    CopyProcessor* m_copyProcessor = nullptr;

    QImage m_pendingFullScreenshot;
    QString m_pendingPromptOverride;

    QString m_pendingPrompt;
    QString m_pendingBase64;
    bool m_retryAfterServiceStart = false;
    bool m_isRetryingAfterSwitch = false;

    // 【新增】记录上一次识别的类型，用于自动复制时的脚本选择
    ContentType m_lastRecognizeType = ContentType::Text;
};

#endif // APPCONTROLLER_H
