#ifndef IMAGEVIEWWIDGET_H
#define IMAGEVIEWWIDGET_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QShortcut>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QEvent>
#include <QVBoxLayout>
#include <QLabel>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QMessageBox>
#include <QFileDialog>
#include <QPushButton>
#include <QDebug>
#include <QPainter>
#include <QResizeEvent>
#include <QTimer>
#include <QtMath>

#include "imageprocessor.h"
#include "settingsmanager.h"

class ImageViewWidget : public QWidget
{
    Q_OBJECT

public:
    enum ViewMode {
        FitToView,
        FitToWidth,
        FitToHeight,
        OriginalSize
    };

    explicit ImageViewWidget(QWidget* parent = nullptr);

    void setImage(const QImage& image);
    void setImage(const QString& path);

    QString currentBase64() const { return m_currentBase64; }
    void clear();
    bool hasImage() const { return !m_currentBase64.isEmpty(); }

    void setViewMode(ViewMode mode);

    QImage currentImage() const;

signals:
    void imageChanged();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

    bool event(QEvent* event) override;

    QImage m_originalImage;

private slots:
    void openImageFile();
    void onFitToViewClicked();
    void onFitToWidthClicked();
    void onFitToHeightClicked();
    void onOriginalSizeClicked();

private:
    void setupUi();
    void updateBase64FromImage(const QImage& image);
    void updateView();

    QGraphicsView* m_view;
    QGraphicsScene* m_scene;
    QGraphicsPixmapItem* m_pixmapItem;
    QString m_currentBase64;

    ViewMode m_currentMode;
    QWidget* m_buttonContainer;
    QPushButton* m_fitToViewBtn;
    QPushButton* m_fitToWidthBtn;
    QPushButton* m_fitToHeightBtn;
    QPushButton* m_originalSizeBtn;

    QPixmap m_originalPixmap;
    bool m_manualZoomMode;

    void updateButtonStyles();
};
#include "imageviewwidget_impl.h"

#endif // IMAGEVIEWWIDGET_H
