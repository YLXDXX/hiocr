#include "floatingball.h"
#include "settingsmanager.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QGuiApplication>
#include <QScreen>
#include <QWindow>

FloatingBall::FloatingBall(QWidget* parent)
// 使用 Qt::Tool 替代 Qt::Dialog，作为不占任务栏的辅助窗口
// 在 Wayland 下，无论是 Tool 还是 Dialog 都无法绕过合成器强制全局置顶
// 这是 Wayland 的安全策略
: QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool | Qt::WindowDoesNotAcceptFocus)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);

    SettingsManager* s = SettingsManager::instance();
    m_size = s->floatingBallSize();
    m_autoHideTime = s->floatingBallAutoHideTime();
    m_alwaysVisible = s->floatingBallAlwaysVisible();
    setFixedSize(m_size, m_size);

    bool isWayland = QGuiApplication::platformName() == "wayland";
    if (!isWayland) {
        int posX = s->floatingBallPosX();
        int posY = s->floatingBallPosY();
        if (posX == -1 || posY == -1) {
            QScreen* screen = QGuiApplication::primaryScreen();
            if (screen) {
                QRect geo = screen->availableGeometry();
                posX = geo.right() - m_size - 20;
                posY = geo.bottom() - m_size - 20;
            }
        }
        move(posX, posY);
    }

    m_autoHideTimer = new QTimer(this);
    m_autoHideTimer->setSingleShot(true);
    connect(m_autoHideTimer, &QTimer::timeout, this, &FloatingBall::onAutoHide);

    m_animationTimer = new QTimer(this);
    m_animationTimer->setInterval(50);
    connect(m_animationTimer, &QTimer::timeout, this, &FloatingBall::onAnimationTick);

    if (m_alwaysVisible) {
        show();
    }
}

void FloatingBall::applySettings(int size, const QPoint& pos, int autoHideTime, bool alwaysVisible)
{
    bool sizeChanged = (m_size != size);
    m_size = size;
    m_autoHideTime = autoHideTime;
    m_alwaysVisible = alwaysVisible;

    if (sizeChanged) {
        setFixedSize(m_size, m_size);
        update();
    }

    bool isWayland = QGuiApplication::platformName() == "wayland";
    if (!isWayland && !pos.isNull()) {
        move(pos);
    }

    if (m_alwaysVisible && m_state == Idle) {
        show();
        update();
    } else if (!m_alwaysVisible && m_state == Idle) {
        hide();
    }
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
            if (m_alwaysVisible) {
                show();
                update();
            } else {
                hide();
            }
            break;
        case Recognizing:
            setToolTip(message.isEmpty() ? QString::fromUtf8("正在识别... (左键截图)") : message);
            m_animationAngle = 0;
            m_animationTimer->start();
            show();
            break;
        case Success:
            setToolTip(message.isEmpty() ? QString::fromUtf8("识别完成 (左键截图，右键查看)") : message);
            show();
            update();
            if (m_autoHideTime > 0) {
                m_autoHideTimer->start(m_autoHideTime);
            }
            break;
        case Error:
            setToolTip(message.isEmpty() ? QString::fromUtf8("识别错误 (左键截图，右键查看)") : message);
            show();
            update();
            if (m_autoHideTime > 0) {
                m_autoHideTimer->start(m_autoHideTime);
            }
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

void FloatingBall::savePosition()
{
    m_savedPos = pos();
    m_needsPositionRestore = true;
}

void FloatingBall::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    raise();

    // 【新增】如果位置在隐藏前被保存过（如截图前），则恢复到原位置
    // 这解决了某些窗口管理器在 show() 时将 Qt::Tool 窗口重置到屏幕中央的问题
    if (m_needsPositionRestore) {
        bool isWayland = QGuiApplication::platformName() == "wayland";
        if (!isWayland && !m_savedPos.isNull()) {
            move(m_savedPos);
        }
        m_needsPositionRestore = false;
    }
}

void FloatingBall::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    qreal margin = 2.0;
    QRectF ballRect(margin, margin, width() - 2 * margin, height() - 2 * margin);

    QColor bgColor;
    bool isIdleAlwaysVisible = (m_state == Idle && m_alwaysVisible);

    if (isIdleAlwaysVisible) {
        bgColor = QColor(100, 100, 100, 180);
    } else {
        switch (m_state) {
            case Recognizing: bgColor = QColor(52, 152, 219); break;
            case Success:     bgColor = QColor(46, 204, 113); break;
            case Error:       bgColor = QColor(231, 76, 60);  break;
            default: return;
        }
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 40));
    painter.drawEllipse(ballRect.translated(1, 1));

    painter.setBrush(bgColor);
    painter.drawEllipse(ballRect);

    painter.setPen(QPen(Qt::white, 2.5));
    painter.setBrush(Qt::NoBrush);

    qreal centerW = width() / 2.0;
    qreal centerH = height() / 2.0;
    QPointF center(centerW, centerH);

    if (isIdleAlwaysVisible) {
        qreal sz = m_size / 4.0;
        QRectF iconRect(centerW - sz, centerH - sz, sz*2, sz*2);
        painter.drawRect(iconRect);
        painter.drawLine(QPointF(iconRect.left(), iconRect.top()), QPointF(iconRect.right(), iconRect.bottom()));
    } else {
        switch (m_state) {
            case Recognizing: {
                QRectF arcRect = ballRect.adjusted(m_size/8, m_size/8, -m_size/8, -m_size/8);
                QPainterPath path;
                path.arcMoveTo(arcRect, m_animationAngle);
                path.arcTo(arcRect, m_animationAngle, 270);
                painter.drawPath(path);
                painter.setPen(Qt::NoPen);
                painter.setBrush(Qt::white);
                painter.drawEllipse(center, m_size/8.0, m_size/8.0);
                break;
            }
            case Success: {
                QPen checkPen(Qt::white, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
                painter.setPen(checkPen);
                QPainterPath checkPath;
                checkPath.moveTo(center.x() - m_size/6, center.y());
                checkPath.lineTo(center.x() - m_size/14, center.y() + m_size/7);
                checkPath.lineTo(center.x() + m_size/5, center.y() - m_size/6);
                painter.drawPath(checkPath);
                break;
            }
            case Error: {
                QPen xPen(Qt::white, 3, Qt::SolidLine, Qt::RoundCap);
                painter.setPen(xPen);
                painter.drawLine(QPointF(center.x() - m_size/7, center.y() - m_size/7),
                                 QPointF(center.x() + m_size/7, center.y() + m_size/7));
                painter.drawLine(QPointF(center.x() + m_size/7, center.y() - m_size/7),
                                 QPointF(center.x() - m_size/7, center.y() + m_size/7));
                break;
            }
            default:
                break;
        }
    }
}

void FloatingBall::mousePressEvent(QMouseEvent* event)
{
    // 右键按住准备拖动
    if (event->button() == Qt::RightButton) {
        m_dragStartPos = event->globalPosition().toPoint() - pos();
        m_dragging = false;
    }
}

void FloatingBall::mouseMoveEvent(QMouseEvent* event)
{
    // 右键按住移动时执行拖动
    if (event->buttons() & Qt::RightButton) {
        QPoint currentPos = event->globalPosition().toPoint();
        QPoint diff = currentPos - pos() - m_dragStartPos;

        if (!m_dragging && diff.manhattanLength() > 5) {
            m_dragging = true;
            QWindow* wnd = windowHandle();
            if (wnd) {
                wnd->startSystemMove();
            }
        }
    }
}

void FloatingBall::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        // 左键点击：触发截图
        emit screenshotTriggered();
    } else if (event->button() == Qt::RightButton) {
        if (m_dragging) {
            // 右键拖动结束：保存位置
            bool isWayland = QGuiApplication::platformName() == "wayland";
            if (!isWayland) {
                emit positionChanged(pos());
            }
        } else {
            // 右键点击（未拖动）：显示主窗口
            emit showWindowTriggered();
        }
    }
    m_dragging = false;
}
