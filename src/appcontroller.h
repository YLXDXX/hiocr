#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H
#include "servicemanager.h"

#include <QObject>
#include <QImage>
#include <QClipboard>
#include <QLocalServer>

class MainWindow;
class ScreenshotManager;
class RecognitionManager;
class SettingsManager;
class TrayManager;
class ShortcutHandler;
class NetworkManager; // 【新增】

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

    // 【新增】服务管理相关槽函数
    void onServiceSelected(const QString& id);
    void toggleService(const QString& id);

    void onNewInstanceConnected();

private:
    void setupManagers();
    void setupConnections();
    void applySettings();
    void setupSingleInstance();

    // 【新增】应用服务配置
    void applyServiceConfig(const QString& serviceId);

    MainWindow* m_mainWindow = nullptr;
    ScreenshotManager* m_screenshotManager = nullptr;
    RecognitionManager* m_recognitionManager = nullptr;
    SettingsManager* m_settings = nullptr;
    TrayManager* m_trayManager = nullptr;
    ShortcutHandler* m_shortcutHandler = nullptr;
    ServiceManager* m_serviceManager = nullptr;

    QLocalServer* m_localServer = nullptr;

    QImage m_pendingFullScreenshot;
    QString m_pendingPromptOverride;

    QString m_pendingPrompt;
    QString m_pendingBase64;
    bool m_retryAfterServiceStart = false;

    // 【新增】重试控制标志
    bool m_isRetryingAfterSwitch = false;
};

#endif // APPCONTROLLER_H
