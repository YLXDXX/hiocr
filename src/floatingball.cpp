#include "floatingball.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QGuiApplication>
#include <QScreen>

FloatingBall::FloatingBall(QWidget* parent)
: QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(48, 48);

    // 初始位置：主屏幕右下角
    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect geo = screen->availableGeometry();
        move(geo.right() - 80, geo.bottom() - 80);
    }

    m_autoHideTimer = new QTimer(this);
    m_autoHideTimer->setSingleShot(true);
    connect(m_autoHideTimer, &QTimer::timeout, this, &FloatingBall::onAutoHide);

    m_animationTimer = new QTimer(this);
    m_animationTimer->setInterval(50);
    connect(m_animationTimer, &QTimer::timeout, this, &FloatingBall::onAnimationTick);

    hide();
}

void FloatingBall::setState(State state, const QString& message)
{
    m_state = state;
    m_message = message;

    m_autoHideTimer->stop();
    m_animationTimer->stop();

    switch (state) {
        case Idle:
            setToolTip(QString());
            hide();
            break;
        case Recognizing:
            setToolTip(message.isEmpty() ? QString::fromUtf8("正在识别... (点击查看)") : message);
            m_animationAngle = 0;
            m_animationTimer->start();
            show();
            break;
        case Success:
            setToolTip(message.isEmpty() ? QString::fromUtf8("识别完成 (点击查看)") : message);
            show();
            update();
            m_autoHideTimer->start(5000);
            break;
        case Error:
            setToolTip(message.isEmpty() ? QString::fromUtf8("识别错误 (点击查看)") : message);
            show();
            update();
            m_autoHideTimer->start(8000);
            break;
    }
}

FloatingBall::State FloatingBall::currentState() const
{
    return m_state;
}

void FloatingBall::onAutoHide()
{
    setState(Idle);
}

void FloatingBall::onAnimationTick()
{
    m_animationAngle = (m_animationAngle + 15) % 360;
    update();
}

void FloatingBall::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRectF ballRect(2, 2, 44, 44);

    QColor bgColor;
    switch (m_state) {
        case Recognizing: bgColor = QColor(52, 152, 219); break;
        case Success:     bgColor = QColor(46, 204, 113); break;
        case Error:       bgColor = QColor(231, 76, 60);  break;
        default: return;
    }

    // 阴影
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 40));
    painter.drawEllipse(ballRect.translated(1, 1));

    // 背景圆
    painter.setBrush(bgColor);
    painter.drawEllipse(ballRect);

    // 状态图标
    painter.setPen(QPen(Qt::white, 2.5));
    painter.setBrush(Qt::NoBrush);

    switch (m_state) {
        case Recognizing: {
            // 旋转弧线
            QRectF arcRect = ballRect.adjusted(6, 6, -6, -6);
            QPainterPath path;
            path.arcMoveTo(arcRect, m_animationAngle);
            path.arcTo(arcRect, m_animationAngle, 270);
            painter.drawPath(path);
            // 中心点
            painter.setPen(Qt::NoPen);
            painter.setBrush(Qt::white);
            painter.drawEllipse(ballRect.center(), 3, 3);
            break;
        }
        case Success: {
            // 对勾
            QPen checkPen(Qt::white, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
            painter.setPen(checkPen);
            QPointF c = ballRect.center();
            QPainterPath checkPath;
            checkPath.moveTo(c.x() - 8, c.y());
            checkPath.lineTo(c.x() - 2, c.y() + 7);
            checkPath.lineTo(c.x() + 9, c.y() - 6);
            painter.drawPath(checkPath);
            break;
        }
        case Error: {
            // 叉号
            QPen xPen(Qt::white, 3, Qt::SolidLine, Qt::RoundCap);
            painter.setPen(xPen);
            QPointF c = ballRect.center();
            painter.drawLine(QPointF(c.x() - 6, c.y() - 6),
                             QPointF(c.x() + 6, c.y() + 6));
            painter.drawLine(QPointF(c.x() + 6, c.y() - 6),
                             QPointF(c.x() - 6, c.y() + 6));
            break;
        }
        default:
            break;
    }
}

void FloatingBall::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragStartPos = event->globalPosition().toPoint() - pos();
        m_dragging = false;
    }
}

void FloatingBall::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton) {
        QPoint currentPos = event->globalPosition().toPoint();
        QPoint diff = currentPos - pos() - m_dragStartPos;
        if (diff.manhattanLength() > 5) {
            m_dragging = true;
        }
        if (m_dragging) {
            move(currentPos - m_dragStartPos);
        }
    }
}

void FloatingBall::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && !m_dragging) {
        emit clicked();
    }
    m_dragging = false;
}
