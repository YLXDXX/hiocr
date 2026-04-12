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
    void clicked();
    void rightClicked();
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
    State m_state = Idle;
    QString m_message;
    QTimer* m_autoHideTimer = nullptr;
    QTimer* m_animationTimer = nullptr;
    QPoint m_dragStartPos;
    bool m_dragging = false;
    int m_animationAngle = 0;

    int m_size = 48;
    int m_autoHideTime = 5000;
    bool m_alwaysVisible = false;
};

#endif // FLOATINGBALL_H
