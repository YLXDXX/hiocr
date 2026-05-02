#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include "servicemanager.h"
#include "copyprocessor.h"
#include "floatingball.h"

#include <QObject>
#include <QImage>
#include <QApplication>
#include <QTimer>
#include <QDebug>
#include <QLocalServer>
#include <QLocalSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStatusBar>

#include "mainwindow.h"
#include "screenshotmanager.h"
#include "recognitionmanager.h"
#include "settingsmanager.h"
#include "traymanager.h"
#include "shortcuthandler.h"
#include "imageprocessor.h"
#include "settingsdialog.h"
#include "singleapplication.h"
#include "historymanager.h"

class MainWindow;
class ScreenshotManager;
class RecognitionManager;
#include "settingsmanager.h"

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
    void takeScreenshot();
    void takeTextRecognizeScreenshot();
    void takeFormulaRecognizeScreenshot();
    void takeTableRecognizeScreenshot();
    void abortRecognition();
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

    void onServiceSelected(const QString& id);
    void toggleService(const QString& id);

    void onSingleInstanceMessageReceived(const QByteArray& message);
    void onManualProcessorTriggered(ContentType type);
    void onSilentNotificationClicked();

    void onFloatingBallPositionChanged(const QPoint& pos);
    void onFloatingBallSettingsChanged();

private:
    void setupManagers();
    void setupConnections();
    void applySettings();
    void setupSingleInstance();
    void applyServiceConfig(const QString& serviceId);

    MainWindow* m_mainWindow = nullptr;
    ScreenshotManager* m_screenshotManager = nullptr;
    RecognitionManager* m_recognitionManager = nullptr;
    SettingsManager* m_settings = nullptr;
    TrayManager* m_trayManager = nullptr;
    ShortcutHandler* m_shortcutHandler = nullptr;
    ServiceManager* m_serviceManager = nullptr;
    CopyProcessor* m_copyProcessor = nullptr;

    QImage m_pendingFullScreenshot;
    QString m_pendingPromptOverride;

    QString m_pendingPrompt;
    QString m_pendingBase64;
    bool m_retryAfterServiceStart = false;
    bool m_isRetryingAfterSwitch = false;

    ContentType m_lastRecognizeType = ContentType::Text;

    QString m_currentServiceName;

    void showSilentNotification(FloatingBall::State state, const QString& message = QString());

    FloatingBall* m_floatingBall = nullptr;
};

#include "appcontroller_impl.h"

#endif // APPCONTROLLER_H
