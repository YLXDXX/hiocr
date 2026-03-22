#include "screenshotareaselector.h"
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QApplication>
#include <QScreen>
#include <QDebug>

ScreenshotAreaSelector::ScreenshotAreaSelector(const QImage& screenshot, QWidget* parent)
: QDialog(parent), m_screenshot(screenshot), m_drawing(false)
{
    QScreen* screen = QApplication::primaryScreen();
    // 获取逻辑几何尺寸（Wayland/X11通用）
    QRect screenRect = screen->geometry();

    // 关键修正：
    // 1. 窗口设置为逻辑大小，覆盖整个屏幕
    setGeometry(screenRect);
    // 或者使用 setFixedSize(screenRect.size());

    // 移除之前的 dpr 手动乘法，避免 Wayland 下窗口尺寸异常

    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    // 注意：Wayland 下可能不支持 Qt::WA_TranslucentBackground 的任意形状透明，
    // 但简单的全屏遮罩通常是支持的。
    setAttribute(Qt::WA_TranslucentBackground);

    setCursor(Qt::CrossCursor);

    // 2. 计算逻辑屏幕到物理图片的缩放比例
    // 图片宽度 / 屏幕逻辑宽度 = 缩放比
    if (screenRect.width() > 0 && screenRect.height() > 0) {
        m_scaleX = static_cast<qreal>(m_screenshot.width()) / screenRect.width();
        m_scaleY = static_cast<qreal>(m_screenshot.height()) / screenRect.height();
    } else {
        m_scaleX = m_scaleY = 1.0;
    }
}

void ScreenshotAreaSelector::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    // 将物理图片绘制到逻辑窗口中
    // Qt 会自动缩放图片以填满 rect()，或者我们可以手动缩放
    // 这里我们希望图片填满整个窗口背景
    QRect targetRect = rect(); // 窗口矩形（逻辑大小）
    painter.drawImage(targetRect, m_screenshot);

    QRect highlightRect = m_drawing ? QRect(m_startPoint, m_endPoint).normalized() : m_selectedRect;
    if (!highlightRect.isEmpty()) {
        painter.setPen(QPen(Qt::red, 2));
        painter.drawRect(highlightRect);
    }
}

void ScreenshotAreaSelector::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_startPoint = event->pos();
        m_endPoint = m_startPoint;
        m_drawing = true;
    }
}

void ScreenshotAreaSelector::mouseMoveEvent(QMouseEvent* event)
{
    if (m_drawing) {
        m_endPoint = event->pos();
        update();
    }
}

void ScreenshotAreaSelector::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_drawing) {
        m_endPoint = event->pos();
        m_selectedRect = QRect(m_startPoint, m_endPoint).normalized();
        m_drawing = false;
        update();
        if (m_selectedRect.width() > 5 && m_selectedRect.height() > 5) {
            accept();
        }
    }
}

void ScreenshotAreaSelector::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        accept();
    } else if (event->key() == Qt::Key_Escape) {
        reject();
    } else {
        QDialog::keyPressEvent(event);
    }
}

// 关键修正：将逻辑坐标选区转换为图片物理坐标选区
QRect ScreenshotAreaSelector::selectedRect() const
{
    // m_selectedRect 是鼠标在窗口（逻辑坐标系）中画出的区域
    // 我们需要将其映射回原始图片的像素坐标

    return QRect(
        qRound(m_selectedRect.x() * m_scaleX),
                 qRound(m_selectedRect.y() * m_scaleY),
                 qRound(m_selectedRect.width() * m_scaleX),
                 qRound(m_selectedRect.height() * m_scaleY)
    );
}
