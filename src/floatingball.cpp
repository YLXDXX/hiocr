#include "floatingball.h"
#include "settingsmanager.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QGuiApplication>
#include <QScreen>
#include <QWindow>

#ifdef HAVE_KF6_WINDOWSYSTEM
#include <KWindowSystem>
#include <KX11Extras>
#include <netwm_def.h>
#include <QDBusInterface>
#include <QDBusReply>
#include <QStandardPaths>
#include <QFile>
#include <QTextStream>
#include <QUuid>
#endif


FloatingBall::FloatingBall(QWidget* parent)
: QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool | Qt::WindowDoesNotAcceptFocus)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);

    // 【新增】设置唯一窗口标题，用于 KWin 脚本在 Wayland 下识别此窗口
    setWindowTitle("hiocr_floating_ball");

    SettingsManager* s = SettingsManager::instance();
    m_size = s->floatingBallSize();
    m_autoHideTime = s->floatingBallAutoHideTime();
    m_alwaysVisible = s->floatingBallAlwaysVisible();
    setFixedSize(m_size, m_size);

    // 从设置读取位置
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

    // 初始化追踪位置
    m_trackedPos = QPoint(posX, posY);

    // 所有平台都调用 move()，Wayland 下可能被合成器忽略，
    // 但 Qt 会在首次 show() 时将此位置作为初始映射位置发送给合成器
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

void FloatingBall::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    raise();
    ensureKeepAbove();  // 【新增】确保置顶

    if (m_needsPositionRestore && !m_trackedPos.isNull()) {
        QTimer::singleShot(50, this, [this](){
            move(m_trackedPos);
        });
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
    if (event->button() == Qt::RightButton) {
        m_dragging = false;
        // 记录右键按下时的光标位置（用于判断拖动阈值）
        m_rightPressStartPos = event->globalPosition().toPoint();
        // 记录拖动起始数据（如果后续触发拖动，这些值会被使用）
        m_dragStartCursorPos = event->globalPosition().toPoint();
        m_dragStartBallPos = m_trackedPos;
    }
}

void FloatingBall::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::RightButton) {
        QPoint currentGlobalPos = event->globalPosition().toPoint();
        QPoint diff = currentGlobalPos - m_rightPressStartPos;

        if (!m_dragging && diff.manhattanLength() > 5) {
            m_dragging = true;
            // 更新拖动起始数据为实际开始拖动时的状态
            m_dragStartCursorPos = currentGlobalPos;
            m_dragStartBallPos = m_trackedPos;

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
        emit screenshotTriggered();
    } else if (event->button() == Qt::RightButton) {
        if (m_dragging) {
            // 【关键修复】Wayland 下 pos() 不可靠，通过光标位移计算新位置
            // startSystemMove() 由合成器处理，光标的屏幕位移 = 窗口的屏幕位移
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

void FloatingBall::ensureKeepAbove()
{
    #ifdef HAVE_KF6_WINDOWSYSTEM
    WId wid = this->winId();
    if (!wid) return;

    if (KWindowSystem::isPlatformX11()) {
        // X11: 使用 KX11Extras（KF6 将 X11 专用方法迁移到了这里）
        KX11Extras::setState(wid, NET::KeepAbove);
    } else {
        // Wayland: 没有公共 C++ API，必须通过 KWin D-Bus 脚本实现
        QTimer::singleShot(100, this, &FloatingBall::requestKeepAboveViaKWin); //迟调用，才能成功捕获到正确的窗口
    }
    #endif
}

void FloatingBall::requestKeepAboveViaKWin() {
    #ifdef HAVE_KF6_WINDOWSYSTEM
    if (KWindowSystem::isPlatformX11()) return;

    QString uniqueId = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
    QString scriptName = "hiocr_keepabove_service_" + uniqueId;
    QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/" + scriptName + ".js";

    // 纯执行逻辑，不带重试
    QString scriptCode =
    "var windows = workspace.windowList();"
    "for (var i = 0; i < windows.length; i++) {"
    "    if (windows[i].caption === 'hiocr_floating_ball' && windows[i].resourceClass === 'hiocr') {"
    "        windows[i].keepAbove = true;"
    "        print('HIOCR_TEST: Found and set KeepAbove: '+windows[i].caption+'  '+windows[i].resourceClass);"
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

    // 延迟清理临时文件（KWin 加载脚本后不再需要该文件）
    QTimer::singleShot(5000, this, [tempPath]() {
        QFile::remove(tempPath);
    });
    #endif
}

