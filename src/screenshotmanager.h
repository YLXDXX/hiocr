#ifndef SCREENSHOTMANAGER_H
#define SCREENSHOTMANAGER_H

#include <QObject>
#include <QImage>
#include <QRect>
#include <QDBusInterface>
#include <QDBusPendingCall>
#include <QDBusPendingReply>
#include <QDBusConnection>
#include <QScreen>
#include <QGuiApplication>
#include <QUrl>
#include <QFile>
#include <QDebug>
#include <QPixmap>

class ScreenshotManager : public QObject
{
    Q_OBJECT

public:
    explicit ScreenshotManager(QObject* parent = nullptr);

    void takeScreenshot();

signals:
    void screenshotCaptured(const QImage& image);
    void screenshotFailed(const QString& errorMessage);

private slots:
    void onPortalResponse(uint response, const QVariantMap& results);

private:
    void requestViaPortal();
    void fallbackScreenshot();

    QImage m_fullScreenshot;
};

// --- Implementation ---

inline ScreenshotManager::ScreenshotManager(QObject* parent)
    : QObject(parent)
{
}

inline void ScreenshotManager::takeScreenshot()
{
    requestViaPortal();
}


inline void ScreenshotManager::requestViaPortal()
{
    if (!QDBusConnection::sessionBus().isConnected()) {
        fallbackScreenshot();
        return;
    }

    QDBusInterface portal("org.freedesktop.portal.Desktop",
                          "/org/freedesktop/portal/desktop",
                          "org.freedesktop.portal.Screenshot",
                          QDBusConnection::sessionBus());

    if (!portal.isValid()) {
        fallbackScreenshot();
        return;
    }

    QVariantMap options;
    options["interactive"] = false;
    options["modal"] = true;
    options["image_format"] = "png";
    options["handle_cursor"] = false;
    options["include_cursor"] = false;

    QDBusPendingReply<QDBusObjectPath> reply = portal.asyncCall("Screenshot", "", options);

    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher* self){
        self->deleteLater();
        QDBusPendingReply<QDBusObjectPath> reply = *self;

        if (reply.isError()) {
            qWarning() << "Portal screenshot failed:" << reply.error().message();
            fallbackScreenshot();
            return;
        }

        QDBusObjectPath path = reply.value();
        QDBusConnection::sessionBus().connect(
            "org.freedesktop.portal.Desktop",
            path.path(),
                                              "org.freedesktop.portal.Request",
                                              "Response",
                                              this,
                                              SLOT(onPortalResponse(uint,QVariantMap))
        );
    });
}

inline void ScreenshotManager::onPortalResponse(uint response, const QVariantMap& results)
{
    if (response != 0) {
        emit screenshotFailed("截图取消或失败");
        return;
    }

    QString uri = results["uri"].toString();
    if (uri.isEmpty()) {
        emit screenshotFailed("未获取到截图 URI");
        return;
    }

    QUrl url(uri);
    QString localPath = url.toLocalFile();

    QImage fullScreenshot;
    QFile file(localPath);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray imageData = file.readAll();
        file.close();

        QFile::remove(localPath);

        if (!fullScreenshot.loadFromData(imageData)) {
            emit screenshotFailed("无法解析截图图像数据");
            return;
        }
    } else {
        QFile::remove(localPath);
        emit screenshotFailed("无法读取截图临时文件");
        return;
    }

    if (fullScreenshot.isNull()) {
        emit screenshotFailed("无法加载截图图像");
        return;
    }

    emit screenshotCaptured(fullScreenshot);
}

inline void ScreenshotManager::fallbackScreenshot()
{
    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen) {
        QPixmap pixmap = screen->grabWindow(0);
        if (!pixmap.isNull()) {
            emit screenshotCaptured(pixmap.toImage());
        } else {
            emit screenshotFailed("无法获取屏幕截图");
        }
    } else {
        emit screenshotFailed("无法获取主屏幕");
    }
}

#endif // SCREENSHOTMANAGER_H
