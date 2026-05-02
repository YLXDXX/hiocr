#ifndef FLOATINGBALL_IMPL_H
#define FLOATINGBALL_IMPL_H

#include "floatingball.h"

// --- Implementation ---

inline FloatingBall::FloatingBall(QWidget* parent)
: QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool | Qt::WindowDoesNotAcceptFocus)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);

    setWindowTitle("hiocr_floating_ball");

    SettingsManager* s = SettingsManager::instance();
    m_size = s->floatingBallSize();
    m_autoHideTime = s->floatingBallAutoHideTime();
    m_alwaysVisible = s->floatingBallAlwaysVisible();
    setFixedSize(m_size, m_size);

    int posX = s->floatingBallPosX();
    int posY = s->floatingBallPosY();
    if (posX < 0 || posY < 0) {
        QScreen* screen = QGuiApplication::primaryScreen();
        if (screen) {
            QRect geo = screen->availableGeometry();
            posX = geo.right() - m_size - 20;
            posY = geo.bottom() - m_size - 20;
        }
    }

    m_trackedPos = QPoint(posX, posY);

    move(posX, posY);

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

inline void FloatingBall::applySettings(int size, const QPoint& pos, int autoHideTime, bool alwaysVisible)
{
    bool sizeChanged = (m_size != size);
    m_size = size;
    m_autoHideTime = autoHideTime;
    m_alwaysVisible = alwaysVisible;

    if (sizeChanged) {
        setFixedSize(m_size, m_size);
        update();
    }

    if (!pos.isNull() && pos.x() >= 0 && pos.y() >= 0) {
        m_trackedPos = pos;
        move(pos);
    }

    if (m_alwaysVisible && m_state == Idle) {
        show();
        update();
    } else if (!m_alwaysVisible && m_state == Idle) {
        hide();
    }
}

inline void FloatingBall::setState(State state, const QString& message)
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

inline FloatingBall::State FloatingBall::currentState() const
{
    return m_state;
}

inline void FloatingBall::onAutoHide()
{
    setState(Idle);
}

inline void FloatingBall::onAnimationTick()
{
    m_animationAngle = (m_animationAngle + 15) % 360;
    update();
}

inline void FloatingBall::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    raise();
    ensureKeepAbove();

    if (m_needsPositionRestore && !m_trackedPos.isNull()) {
        QTimer::singleShot(50, this, [this](){
            move(m_trackedPos);
        });
        m_needsPositionRestore = false;
    }
}

inline void FloatingBall::paintEvent(QPaintEvent* event)
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

inline void FloatingBall::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton) {
        m_dragging = false;
        m_rightPressStartPos = event->globalPosition().toPoint();
        m_dragStartCursorPos = event->globalPosition().toPoint();
        m_dragStartBallPos = m_trackedPos;
    }
}

inline void FloatingBall::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::RightButton) {
        QPoint currentGlobalPos = event->globalPosition().toPoint();
        QPoint diff = currentGlobalPos - m_rightPressStartPos;

        if (!m_dragging && diff.manhattanLength() > 5) {
            m_dragging = true;
            m_dragStartCursorPos = currentGlobalPos;
            m_dragStartBallPos = m_trackedPos;

            QWindow* wnd = windowHandle();
            if (wnd) {
                wnd->startSystemMove();
            }
        }
    }
}

inline void FloatingBall::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit screenshotTriggered();
    } else if (event->button() == Qt::RightButton) {
        if (m_dragging) {
            QPoint currentCursorPos = QCursor::pos();
            QPoint cursorDelta = currentCursorPos - m_dragStartCursorPos;
            m_trackedPos = m_dragStartBallPos + cursorDelta;

            emit positionChanged(m_trackedPos);
        } else {
            emit showWindowTriggered();
        }
    }
    m_dragging = false;
}

inline void FloatingBall::ensureKeepAbove()
{
    #ifdef HAVE_KF6_WINDOWSYSTEM
    WId wid = this->winId();
    if (!wid) return;

    if (KWindowSystem::isPlatformX11()) {
        KX11Extras::setState(wid, NET::KeepAbove);
        KX11Extras::setType(wid, NET::Notification);
    } else {
        QTimer::singleShot(100, this, &FloatingBall::requestKeepAboveViaKWin);
    }
    #endif
}

inline void FloatingBall::requestKeepAboveViaKWin() {
    #ifdef HAVE_KF6_WINDOWSYSTEM
    if (KWindowSystem::isPlatformX11()) return;

    QString uniqueId = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
    QString scriptName = "hiocr_keepabove_service_" + uniqueId;
    QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/" + scriptName + ".js";

    QString scriptCode =
    "var windows = workspace.windowList();"
    "for (var i = 0; i < windows.length; i++) {"
    "    if (windows[i].caption === 'hiocr_floating_ball' && windows[i].resourceClass === 'hiocr') {"
    "        windows[i].keepAbove = true;"
    "        windows[i].skipTaskbar = true;"
    "        windows[i].skipSwitcher = true;"
    "        print('HIOCR_TEST: Set window properties: '+windows[i].caption+'  '+windows[i].resourceClass);"
    "        break;"
    "    }"
    "}";

    QFile file(tempPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(scriptCode.toUtf8());
        file.close();
    }

    QDBusInterface scripting("org.kde.KWin", "/Scripting", "org.kde.kwin.Scripting");
    QDBusReply<int> reply = scripting.call("loadScript", tempPath, scriptName);

    if (reply.isValid() && reply.value() != -1) {
        QDBusInterface instance("org.kde.KWin", "/Scripting/Script" + QString::number(reply.value()), "org.kde.kwin.Script");
        instance.call("run");
    }

    QTimer::singleShot(5000, this, [tempPath]() {
        QFile::remove(tempPath);
    });
    #endif
}

#endif // FLOATINGBALL_IMPL_H
