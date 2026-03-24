#ifndef SCREENSHOTAREASELECTOR_H
#define SCREENSHOTAREASELECTOR_H

#include <QDialog>
#include <QImage>
#include <QScreen>

class ScreenshotAreaSelector : public QDialog
{
    Q_OBJECT

public:
    // 【修改】增加 screen 参数，指明截图来源屏幕
    explicit ScreenshotAreaSelector(const QImage& screenshot, QScreen* screen, QWidget* parent = nullptr);
    QRect selectedRect() const;

protected:
    void paintEvent(QPaintEvent* event) override;

    // 【修复】添加 void 返回类型
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    QImage m_screenshot;

    // 逻辑坐标系下的选区（用于绘制交互）
    QRect m_selectedRect;
    QPoint m_startPoint;
    QPoint m_endPoint;
    bool m_drawing;

    // 用于坐标转换的比率
    qreal m_scaleX = 1.0;
    qreal m_scaleY = 1.0;
};

#endif // SCREENSHOTAREASELECTOR_H
