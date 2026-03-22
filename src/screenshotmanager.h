#ifndef SCREENSHOTMANAGER_H
#define SCREENSHOTMANAGER_H

#include <QObject>
#include <QImage>
#include <QRect>

class ScreenshotManager : public QObject
{
    Q_OBJECT

public:
    explicit ScreenshotManager(QObject* parent = nullptr);

    // 开始截图流程（通过 Portal 或降级方案）
    void takeScreenshot();

signals:
    // 截图成功并完成区域选择后发出，携带裁剪后的图像
    void screenshotCaptured(const QImage& image);
    // 截图失败时发出
    void screenshotFailed(const QString& errorMessage);

private slots:
    void onPortalResponse(uint response, const QVariantMap& results);

private:
    void requestViaPortal();
    void fallbackScreenshot();

    QImage m_fullScreenshot;  // 保存全屏截图，用于区域选择
};

#endif // SCREENSHOTMANAGER_H
