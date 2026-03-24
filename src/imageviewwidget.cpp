#include "imageviewwidget.h"
#include "imageprocessor.h"
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

ImageViewWidget::ImageViewWidget(QWidget* parent) : QWidget(parent), m_currentMode(FitToView), m_manualZoomMode(false)
{
    setupUi();
    m_view->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_view->setInteractive(false);
}

void ImageViewWidget::setupUi()
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    QLabel* hint = new QLabel("提示：可按 Ctrl+V 粘贴剪贴板图片", this);
    hint->setStyleSheet("color: gray; font-size: 12px;");
    layout->addWidget(hint);

    m_view = new QGraphicsView(this);
    m_scene = new QGraphicsScene(this);
    m_view->setScene(m_scene);

    // 开启平滑变换和抗锯齿，保证缩放时的显示效果
    m_view->setRenderHint(QPainter::SmoothPixmapTransform, true);
    m_view->setRenderHint(QPainter::Antialiasing, true);
    m_view->setRenderHint(QPainter::TextAntialiasing, true);

    m_view->setDragMode(QGraphicsView::ScrollHandDrag);
    m_view->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    m_view->setBackgroundBrush(QColor("#f0f0f0"));

    // 【修改】优化样式表，增加 QGraphicsView 前缀，并简化代码
    m_view->setStyleSheet(
        "QGraphicsView { border: 1px solid gray; background-color: #f0f0f0; }"
        "QScrollBar:vertical { width: 8px; background: #f0f0f0; margin: 0px; }"
        "QScrollBar::handle:vertical { background: #c0c0c0; border-radius: 4px; min-height: 20px; }"
        "QScrollBar:horizontal { height: 8px; background: #f0f0f0; margin: 0px; }"
        "QScrollBar::handle:horizontal { background: #c0c0c0; border-radius: 4px; min-width: 20px; }"
        "QScrollBar::add-line, QScrollBar::sub-line { border: none; background: none; }"
        "QScrollBar::add-page, QScrollBar::sub-page { background: none; }"
    );

    m_view->setFocusPolicy(Qt::StrongFocus);
    layout->addWidget(m_view);

    m_pixmapItem = nullptr;

    // 创建按钮容器（浮动在右下角）
    m_buttonContainer = new QWidget(this);
    m_buttonContainer->setStyleSheet("background-color: rgba(200, 200, 200, 80); border-radius: 4px;");
    QHBoxLayout* btnLayout = new QHBoxLayout(m_buttonContainer);
    btnLayout->setContentsMargins(4, 4, 4, 4);
    btnLayout->setSpacing(4);

    m_fitToViewBtn = new QPushButton("整页");
    m_fitToWidthBtn = new QPushButton("同宽");
    m_fitToHeightBtn = new QPushButton("同高");
    m_originalSizeBtn = new QPushButton("原大", this);

    QString buttonStyle = "QPushButton {"
    "  background-color: rgba(100, 100, 100, 70);"
    "  color: white;"
    "  border: none;"
    "  border-radius: 3px;"
    "  padding: 2px;"
    "}"
    "QPushButton:hover {"
    "  background-color: rgba(120, 120, 120, 100);"
    "}"
    "QPushButton:pressed {"
    "  background-color: rgba(80, 80, 80, 130);"
    "}";
    m_fitToViewBtn->setStyleSheet(buttonStyle);
    m_fitToWidthBtn->setStyleSheet(buttonStyle);
    m_fitToHeightBtn->setStyleSheet(buttonStyle);
    m_originalSizeBtn->setStyleSheet(buttonStyle);

    m_fitToViewBtn->setFixedSize(40, 24);
    m_fitToWidthBtn->setFixedSize(40, 24);
    m_fitToHeightBtn->setFixedSize(40, 24);
    m_originalSizeBtn->setFixedSize(40, 24);

    btnLayout->addWidget(m_fitToViewBtn);
    btnLayout->addWidget(m_fitToWidthBtn);
    btnLayout->addWidget(m_fitToHeightBtn);
    btnLayout->addWidget(m_originalSizeBtn);
    m_buttonContainer->adjustSize();

    connect(m_fitToViewBtn, &QPushButton::clicked, this, &ImageViewWidget::onFitToViewClicked);
    connect(m_fitToWidthBtn, &QPushButton::clicked, this, &ImageViewWidget::onFitToWidthClicked);
    connect(m_fitToHeightBtn, &QPushButton::clicked, this, &ImageViewWidget::onFitToHeightClicked);
    connect(m_originalSizeBtn, &QPushButton::clicked, this, &ImageViewWidget::onOriginalSizeClicked);

    m_buttonContainer->raise();
}

void ImageViewWidget::setImage(const QImage& image)
{
    if (image.isNull()) {
        QMessageBox::warning(this, "错误", "图片无效");
        return;
    }

    // 关键修改：直接从原图创建 QPixmap，不做任何缩放，保留所有像素细节
    m_originalPixmap = QPixmap::fromImage(image);

    m_manualZoomMode = false;

    // 更新场景
    m_scene->clear();
    m_pixmapItem = m_scene->addPixmap(m_originalPixmap);
    m_scene->setSceneRect(m_originalPixmap.rect());

    updateBase64FromImage(image);
    updateView(); // 应用视图变换
    emit imageChanged();

    m_view->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    m_view->setInteractive(true);
}

void ImageViewWidget::setImage(const QString& path)
{
    QImage image = ImageProcessor::loadImage(path);
    if (image.isNull()) {
        QMessageBox::warning(this, "错误", "无法加载图片: " + path);
        return;
    }
    setImage(image);
}

void ImageViewWidget::clear()
{
    m_scene->clear();
    m_pixmapItem = nullptr;
    m_originalPixmap = QPixmap();
    m_currentBase64.clear();
    emit imageChanged();

    m_view->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_view->setInteractive(false);
}

void ImageViewWidget::setViewMode(ViewMode mode)
{
    m_currentMode = mode;
    m_manualZoomMode = false;
    updateView();
}

void ImageViewWidget::updateView()
{
    if (m_originalPixmap.isNull()) return;

    QRectF viewRect = m_view->viewport()->rect();
    if (viewRect.isEmpty()) {
        QTimer::singleShot(0, this, &ImageViewWidget::updateView);
        return;
    }

    // 如果是手动缩放模式，不重置变换，保留当前状态
    if (m_manualZoomMode) return;

    // 重置变换矩阵，准备计算新的缩放
    m_view->resetTransform();

    QRectF pixRect = m_originalPixmap.rect();
    qreal scaleX = viewRect.width() / pixRect.width();
    qreal scaleY = viewRect.height() / pixRect.height();

    switch (m_currentMode) {
        case FitToView: {
            qreal scale = qMin(scaleX, scaleY);
            m_view->scale(scale, scale);
            break;
        }
        case FitToWidth: {
            m_view->scale(scaleX, scaleX);
            break;
        }
        case FitToHeight: {
            m_view->scale(scaleY, scaleY);
            break;
        }
        case OriginalSize: {
            // 【修复】适配高 DPI 屏幕
            // 目标：让图片的一个像素对应屏幕的一个物理像素
            // 原理：
            // 1. QPixmap 的尺寸是逻辑尺寸，如果图片 DPR = 1.0，则其逻辑尺寸 = 物理尺寸。
            // 2. 屏幕有一个 DPR (例如 1.5)。
            // 3. 如果不缩放，Qt 会把图片的逻辑像素当作屏幕的逻辑像素绘制，
            //    导致物理尺寸变大 (乘以了屏幕 DPR)。
            // 4. 我们需要对视图进行缩小，缩放比例 = 图片DPR / 屏幕DPR。
            //    这样显示出来的逻辑尺寸变小了，但对应的物理像素数正好等于图片原始像素数。

            qreal screenDpr = m_view->devicePixelRatioF(); // 获取屏幕 DPR (如 1.5)
            qreal imageDpr = m_originalPixmap.devicePixelRatio(); // 获取图片 DPR (通常为 1.0)

            qreal factor = imageDpr / screenDpr;

            if (!qFuzzyCompare(factor, 1.0)) {
                m_view->scale(factor, factor);
            }
            break;
        }
    }

    // 将图片居中显示
    m_view->centerOn(m_pixmapItem);
}

void ImageViewWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    if (hasImage()) {
        updateView();
    }
}

void ImageViewWidget::updateBase64FromImage(const QImage& image)
{
    m_currentBase64 = ImageProcessor::imageToBase64(image);
}

void ImageViewWidget::mousePressEvent(QMouseEvent* event)
{
    if (!hasImage() && event->button() == Qt::LeftButton) {
        openImageFile();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}


void ImageViewWidget::wheelEvent(QWheelEvent* event)
{
    if (hasImage() && (event->modifiers() & Qt::ControlModifier)) {
        // 进入手动缩放模式
        m_manualZoomMode = true;

        // 执行缩放
        QPointF scenePos = m_view->mapToScene(event->position().toPoint());
        bool onImage = m_pixmapItem && m_pixmapItem->contains(scenePos);

        QGraphicsView::ViewportAnchor oldAnchor = m_view->transformationAnchor();
        if (onImage)
            m_view->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        else
            m_view->setTransformationAnchor(QGraphicsView::AnchorViewCenter);

        qreal factor = (event->angleDelta().y() > 0) ? 1.1 : 0.9;
        m_view->scale(factor, factor);

        m_view->setTransformationAnchor(oldAnchor);
        event->accept();
        return;
    }
    QWidget::wheelEvent(event);
}

void ImageViewWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (m_buttonContainer) {
        int x = width() - m_buttonContainer->width() - 10;
        int y = height() - m_buttonContainer->height() - 10;
        m_buttonContainer->move(x, y);
    }
    // 如果不是手动模式，窗口大小改变时重新适应
    if (hasImage() && !m_manualZoomMode) {
        updateView();
    }
}

void ImageViewWidget::openImageFile()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("选择图片文件"),
                                                    QString(),
                                                    tr("Images (*.png *.jpg *.jpeg *.bmp *.gif)"));
    if (!fileName.isEmpty()) {
        setImage(fileName);
    }
}

void ImageViewWidget::onFitToViewClicked()
{
    setViewMode(FitToView);
}

void ImageViewWidget::onFitToWidthClicked()
{
    setViewMode(FitToWidth);
}

void ImageViewWidget::onFitToHeightClicked()
{
    setViewMode(FitToHeight);
}

void ImageViewWidget::onOriginalSizeClicked()
{
    setViewMode(OriginalSize);
}
