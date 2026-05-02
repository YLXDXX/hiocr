#ifndef SCREENSHOTAREASELECTOR_H
#define SCREENSHOTAREASELECTOR_H

#include <QDialog>
#include <QImage>
#include <QScreen>
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QApplication>
#include <QDebug>

class ScreenshotAreaSelector : public QDialog
{
    Q_OBJECT

public:
    explicit ScreenshotAreaSelector(const QImage& screenshot, QScreen* screen, QWidget* parent = nullptr);
    QRect selectedRect() const;

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    QImage m_screenshot;

    QRect m_selectedRect;
    QPoint m_startPoint;
    QPoint m_endPoint;
    bool m_drawing;

    qreal m_scaleX = 1.0;
    qreal m_scaleY = 1.0;
};

// --- Implementation ---

inline ScreenshotAreaSelector::ScreenshotAreaSelector(const QImage& screenshot, QScreen* screen, QWidget* parent)
: QDialog(parent), m_screenshot(screenshot), m_drawing(false)
{
    if (!screen) {
        screen = QApplication::primaryScreen();
    }

    QRect screenGeometry = screen->geometry();
    setGeometry(screenGeometry);

    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setCursor(Qt::CrossCursor);

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

inline void ScreenshotAreaSelector::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);

    painter.drawImage(rect(), m_screenshot);

    QRect drawRect = m_drawing ? QRect(m_startPoint, m_endPoint).normalized() : m_selectedRect;

    if (!drawRect.isEmpty()) {
        QPen pen(Qt::red, 2);
        painter.setPen(pen);
        painter.drawRect(drawRect);
    }
}

inline void ScreenshotAreaSelector::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_startPoint = event->pos();
        m_endPoint = m_startPoint;
        m_drawing = true;
        update();
    }
}

inline void ScreenshotAreaSelector::mouseMoveEvent(QMouseEvent* event)
{
    if (m_drawing) {
        m_endPoint = event->pos();
        update();
    }
}

inline void ScreenshotAreaSelector::mouseReleaseEvent(QMouseEvent* event)
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

inline void ScreenshotAreaSelector::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        accept();
    } else if (event->key() == Qt::Key_Escape) {
        reject();
    } else {
        QDialog::keyPressEvent(event);
    }
}

inline QRect ScreenshotAreaSelector::selectedRect() const
{
    return QRect(
        qRound(m_selectedRect.x() * m_scaleX),
                 qRound(m_selectedRect.y() * m_scaleY),
                 qRound(m_selectedRect.width() * m_scaleX),
                 qRound(m_selectedRect.height() * m_scaleY)
    );
}

#endif // SCREENSHOTAREASELECTOR_H
