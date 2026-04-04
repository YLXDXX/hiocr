#ifndef IMAGEVIEWWIDGET_H
#define IMAGEVIEWWIDGET_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QShortcut>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QEvent> // 【新增】引入 QEvent

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

    QImage currentImage() const; // 【新增】获取当前显示的图片对象

signals:
    void imageChanged();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

    // 【新增】重写 event 方法捕获 DPI 变化
    bool event(QEvent* event) override;

    QImage m_originalImage; // 【新增】缓存原始 QImage，用于复制时获取原图数据

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
    bool m_manualZoomMode;      // 是否处于手动缩放模式

    void updateButtonStyles();
};

#endif // IMAGEVIEWWIDGET_H
