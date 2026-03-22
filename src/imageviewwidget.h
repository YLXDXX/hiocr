#ifndef IMAGEVIEWWIDGET_H
#define IMAGEVIEWWIDGET_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QShortcut>
#include <QMouseEvent>
#include <QWheelEvent>

class QPushButton;

class ImageViewWidget : public QWidget
{
    Q_OBJECT

public:
    enum ViewMode {
        FitToView,   // 整页显示
        FitToWidth,  // 宽度适应
        FitToHeight,  // 高度适应
        OriginalSize // 原始大小（1:1）
    };

    explicit ImageViewWidget(QWidget* parent = nullptr);

    void setImage(const QImage& image);
    void setImage(const QString& path);
    QString currentBase64() const { return m_currentBase64; }
    void clear();
    bool hasImage() const { return !m_currentBase64.isEmpty(); }

    void setViewMode(ViewMode mode);   // 设置显示模式

signals:
    void imageChanged();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;   // 重写以调整按钮位置
    void showEvent(QShowEvent* event) override;

private slots:
    void openImageFile();
    void onFitToViewClicked();
    void onFitToWidthClicked();
    void onFitToHeightClicked();
    void onOriginalSizeClicked();

private:
    void setupUi();
    void updateBase64FromImage(const QImage& image);
    void updateView();                 // 根据当前模式更新视图

    QGraphicsView* m_view;
    QGraphicsScene* m_scene;
    QGraphicsPixmapItem* m_pixmapItem;
    QString m_currentBase64;

    // 显示模式相关
    ViewMode m_currentMode;
    QWidget* m_buttonContainer;
    QPushButton* m_fitToViewBtn;
    QPushButton* m_fitToWidthBtn;
    QPushButton* m_fitToHeightBtn;
    QPushButton* m_originalSizeBtn;

    QPixmap m_originalPixmap;   // 保留原始图像，用于高质量缩放
    // 移除 m_displayPixmap，直接使用原图配合视图变换
    bool m_manualZoomMode;      // 是否处于手动缩放模式
};

#endif // IMAGEVIEWWIDGET_H
