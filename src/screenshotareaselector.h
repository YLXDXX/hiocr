#ifndef SCREENSHOTAREASELECTOR_H
#define SCREENSHOTAREASELECTOR_H

#include <QDialog>
#include <QImage>

class ScreenshotAreaSelector : public QDialog
{
    Q_OBJECT

public:
    explicit ScreenshotAreaSelector(const QImage& screenshot, QWidget* parent = nullptr);
    QRect selectedRect() const;

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    QImage m_screenshot;
    QRect m_selectedRect;      // 逻辑坐标系下的选区（用于绘制）
    QPoint m_startPoint;
    QPoint m_endPoint;
    bool m_drawing;

    // 新增：用于坐标转换
    qreal m_scaleX;
    qreal m_scaleY;
};

#endif // SCREENSHOTAREASELECTOR_H
