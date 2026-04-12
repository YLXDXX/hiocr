#ifndef FLOATINGBALL_H
#define FLOATINGBALL_H

#include <QWidget>
#include <QTimer>

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
    void ensureKeepAbove();          // 【新增】确保窗口置顶（Wayland 兼容）
    void requestKeepAboveViaKWin();  // 【新增】通过 KWin D-Bus 脚本设置置顶

    State m_state = Idle;
    QString m_message;
    QTimer* m_autoHideTimer = nullptr;
    QTimer* m_animationTimer = nullptr;
    bool m_dragging = false;
    int m_animationAngle = 0;

    int m_size = 48;
    int m_autoHideTime = 5000;
    bool m_alwaysVisible = false;

    // 位置追踪
    QPoint m_trackedPos;
    bool m_needsPositionRestore = false;

    // 拖动追踪
    QPoint m_dragStartCursorPos;
    QPoint m_dragStartBallPos;

    // 拖动起点
    QPoint m_rightPressStartPos;

    // 【新增】KWin 脚本 ID，避免重复加载
    int m_kwinScriptId = -1;
};

#endif // FLOATINGBALL_H
