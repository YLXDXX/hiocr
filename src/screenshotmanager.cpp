#include "screenshotmanager.h"

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

ScreenshotManager::ScreenshotManager(QObject* parent)
    : QObject(parent)
{
}

void ScreenshotManager::takeScreenshot()
{
    // 优先使用 Portal 方式
    requestViaPortal();
}

void ScreenshotManager::requestViaPortal()
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
    options["interactive"] = false;   // 让用户选择区域或窗口
    options["modal"] = true;
    options["image_format"] = "png";
    options["handle_cursor"] = false; // 设置为 false 以隐藏截图中的鼠标光标
    options["include_cursor"] = false; // 尝试此备选键值


    QDBusPendingReply<QDBusObjectPath> reply = portal.call("Screenshot", "", options);
    reply.waitForFinished();

    if (reply.isError()) {
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
}

void ScreenshotManager::onPortalResponse(uint response, const QVariantMap& results)
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
    QImage fullScreenshot(localPath);

    // 删除临时文件
    QFile::remove(localPath);

    if (fullScreenshot.isNull()) {
        emit screenshotFailed("无法加载截图图像");
        return;
    }

    // 【修改】直接发出全屏截图信号，不再在此处弹出选区对话框
    emit screenshotCaptured(fullScreenshot);
}

void ScreenshotManager::fallbackScreenshot()
{
    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen) {
        QPixmap pixmap = screen->grabWindow(0);
        if (!pixmap.isNull()) {
            // 【修改】直接发出全屏截图信号
            emit screenshotCaptured(pixmap.toImage());
        } else {
            emit screenshotFailed("无法获取屏幕截图");
        }
    } else {
        emit screenshotFailed("无法获取主屏幕");
    }
}
