#include "screenshotmanager.h"
#include "screenshotareaselector.h"

#include <QDBusInterface>
#include <QDBusPendingCall>
#include <QDBusPendingReply>
#include <QDBusConnection>
#include <QScreen>
#include <QGuiApplication>
#include <QUrl>
#include <QFile>
#include <QDebug>

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
        qDebug() << "Screenshot cancelled or failed (response code:" << response << ")";
        emit screenshotFailed("截图取消或失败");
        return;
    }

    QString uri = results["uri"].toString();
    if (uri.isEmpty()) {
        qWarning() << "Screenshot response missing uri";
        emit screenshotFailed("未获取到截图 URI");
        return;
    }

    QUrl url(uri);
    QString localPath = url.toLocalFile();
    QImage fullScreenshot(localPath);
    if (fullScreenshot.isNull()) {
        qWarning() << "Failed to load screenshot image from:" << localPath;
        emit screenshotFailed("无法加载截图图像");
        return;
    }

    // 删除临时文件
    QFile::remove(localPath);

    // 弹出区域选择对话框
    ScreenshotAreaSelector selector(fullScreenshot, nullptr);
    if (selector.exec() == QDialog::Accepted) {
        QRect rect = selector.selectedRect();
        if (!rect.isNull() && rect.isValid()) {
            QImage cropped = fullScreenshot.copy(rect);
            emit screenshotCaptured(cropped);
        } else {
            emit screenshotFailed("选区无效");
        }
    } else {
        // 用户取消
        emit screenshotFailed("截图已取消");
    }
}

void ScreenshotManager::fallbackScreenshot()
{
    // 传统方式（仅 X11 有效）
    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen) {
        QPixmap pixmap = screen->grabWindow(0);
        if (!pixmap.isNull()) {
            // 同样弹出区域选择对话框
            QImage fullScreenshot = pixmap.toImage();
            ScreenshotAreaSelector selector(fullScreenshot, nullptr);
            if (selector.exec() == QDialog::Accepted) {
                QRect rect = selector.selectedRect();
                if (!rect.isNull() && rect.isValid()) {
                    QImage cropped = fullScreenshot.copy(rect);
                    emit screenshotCaptured(cropped);
                } else {
                    emit screenshotFailed("选区无效");
                }
            } else {
                emit screenshotFailed("截图已取消");
            }
        } else {
            emit screenshotFailed("无法获取屏幕截图");
        }
    } else {
        emit screenshotFailed("无法获取主屏幕");
    }
}
