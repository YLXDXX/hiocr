#ifndef FLOATINGBALL_H
#define FLOATINGBALL_H

#include <QWidget>
#include <QTimer>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QGuiApplication>
#include <QScreen>
#include <QWindow>
#include <QStandardPaths>
#include <QFile>
#include <QTextStream>
#include <QUuid>

#ifdef HAVE_KF6_WINDOWSYSTEM
#include <KWindowSystem>
#include <KX11Extras>
#include <netwm_def.h>
#include <QDBusInterface>
#include <QDBusReply>
#endif

#include "settingsmanager.h"

class FloatingBall : public QWidget
{
    Q_OBJECT

public:
    enum State {
        Idle,
        Recognizing,
        Success,
        Error
    };

    explicit FloatingBall(QWidget* parent = nullptr);

    void setState(State state, const QString& message = QString());
    State currentState() const;

    void applySettings(int size, const QPoint& pos, int autoHideTime, bool alwaysVisible);

signals:
    void screenshotTriggered();
    void showWindowTriggered();
    void positionChanged(const QPoint& pos);

protected:
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private slots:
    void onAutoHide();
    void onAnimationTick();

private:
    void ensureKeepAbove();
    void requestKeepAboveViaKWin();

    State m_state = Idle;
    QString m_message;
    QTimer* m_autoHideTimer = nullptr;
    QTimer* m_animationTimer = nullptr;
    bool m_dragging = false;
    int m_animationAngle = 0;

    int m_size = 48;
    int m_autoHideTime = 5000;
    bool m_alwaysVisible = false;

    QPoint m_trackedPos;
    bool m_needsPositionRestore = false;

    QPoint m_dragStartCursorPos;
    QPoint m_dragStartBallPos;

    QPoint m_rightPressStartPos;

    int m_kwinScriptId = -1;
};
#include "floatingball_impl.h"

#endif // FLOATINGBALL_H
