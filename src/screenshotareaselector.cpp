#include "screenshotareaselector.h"
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QApplication>
#include <QDebug>

// 【修改】接受 QScreen 指针
ScreenshotAreaSelector::ScreenshotAreaSelector(const QImage& screenshot, QScreen* screen, QWidget* parent)
: QDialog(parent), m_screenshot(screenshot), m_drawing(false)
{
    // 如果未指定屏幕，默认使用主屏幕
    if (!screen) {
        screen = QApplication::primaryScreen();
    }

    // 关键步骤1：设置窗口几何尺寸为屏幕的逻辑尺寸
    // screen->geometry() 返回的是经过 DPI 缩放后的逻辑尺寸（例如 1920x1080 在 2x 缩放下逻辑尺寸可能是 960x540，取决于平台）
    // 但在某些 Wayland 环境下，这可能仍返回物理尺寸。我们需要一套稳健的换算逻辑。

    QRect screenGeometry = screen->geometry();
    setGeometry(screenGeometry);

    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setCursor(Qt::CrossCursor);

    // 关键步骤2：计算缩放比率
    // 比率 = 物理图像尺寸 / 逻辑窗口尺寸
    // 无论 Wayland 还是 X11，无论是否整数缩放，这个比率决定了如何将鼠标坐标映射回图片像素

    if (screenGeometry.width() > 0 && screenGeometry.height() > 0) {
        m_scaleX = static_cast<qreal>(m_screenshot.width()) / screenGeometry.width();
        m_scaleY = static_cast<qreal>(m_screenshot.height()) / screenGeometry.height();
    } else {
        qWarning() << "Screen geometry is invalid, defaulting scale to 1.0";
        m_scaleX = 1.0;
        m_scaleY = 1.0;
    }

    qDebug() << "Selector Logic Size:" << screenGeometry.size()
    << "Image Physical Size:" << m_screenshot.size()
    << "Scale Factors:" << m_scaleX << m_scaleY;
}

void ScreenshotAreaSelector::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);

    // 1. 绘制背景截图
    // Qt 会自动将物理像素图缩放绘制到逻辑窗口的 rect() 上
    // 这一步非常关键，它会自动适应高 DPI 和非整数缩放
    painter.drawImage(rect(), m_screenshot);

    // 2. 绘制选区边框
    // 选区是在逻辑坐标系中绘制的（鼠标事件也是逻辑坐标）
    QRect drawRect = m_drawing ? QRect(m_startPoint, m_endPoint).normalized() : m_selectedRect;

    if (!drawRect.isEmpty()) {
        // 绘制半透明遮罩（可选，增强视觉效果）
        // painter.fillRect(rect(), QColor(0, 0, 0, 100)); // 如果需要全屏遮罩

        // 绘制红色边框
        // 建议：笔宽使用逻辑值，无需乘以 DPR，Qt 会自动处理线宽
        QPen pen(Qt::red, 2);
        painter.setPen(pen);
        painter.drawRect(drawRect);
    }
}

void ScreenshotAreaSelector::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        // event->pos() 返回的是逻辑坐标
        m_startPoint = event->pos();
        m_endPoint = m_startPoint;
        m_drawing = true;
        update();
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

        // 增加最小尺寸判断，防止误点击
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

// 关键步骤3：坐标转换
QRect ScreenshotAreaSelector::selectedRect() const
{
    // 将逻辑坐标选区转换为图片物理像素选区
    // 使用 qRound 四舍五入，保证像素对齐
    return QRect(
        qRound(m_selectedRect.x() * m_scaleX),
                 qRound(m_selectedRect.y() * m_scaleY),
                 qRound(m_selectedRect.width() * m_scaleX),
                 qRound(m_selectedRect.height() * m_scaleY)
    );
}
