#include "imageviewwidget.h"
#include "imageprocessor.h"
#include "settingsmanager.h" // 【新增】引入设置管理器

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

// 【新增】实现 event 方法，捕获屏幕变化和 DPI 变化
bool ImageViewWidget::event(QEvent* event)
{
    // 监听屏幕变化或 DPI 变化事件
    if (event->type() == QEvent::ScreenChangeInternal ||
        event->type() == QEvent::DevicePixelRatioChange) {

        // 当 DPI 变化时，强制更新视图以修正缩放比例
        if (hasImage() && !m_manualZoomMode) {
            updateView();
        }
        }
        return QWidget::event(event);
}

ImageViewWidget::ImageViewWidget(QWidget* parent) : QWidget(parent), m_manualZoomMode(false)
{
    setupUi();

    // 【修改】从设置加载上次的模式
    int modeIndex = SettingsManager::instance()->imageViewMode();
    // 确保值在有效范围内
    if (modeIndex >= 0 && modeIndex <= 3) {
        m_currentMode = static_cast<ViewMode>(modeIndex);
    } else {
        m_currentMode = FitToView; // 默认值
    }

    // 【新增】更新按钮的高亮状态
    updateButtonStyles();

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

    // 这里设置的基础样式会被 updateButtonStyles() 覆盖，但设置大小等属性保留
    QString buttonBaseStyle = "QPushButton { color: white; border: none; border-radius: 3px; padding: 2px; }";

    m_fitToViewBtn->setStyleSheet(buttonBaseStyle);
    m_fitToWidthBtn->setStyleSheet(buttonBaseStyle);
    m_fitToHeightBtn->setStyleSheet(buttonBaseStyle);
    m_originalSizeBtn->setStyleSheet(buttonBaseStyle);

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
    if (m_currentMode == mode && !m_manualZoomMode) return; // 避免重复设置

    m_currentMode = mode;
    m_manualZoomMode = false;

    // 【新增】保存设置
    SettingsManager::instance()->setImageViewMode(static_cast<int>(mode));

    // 【新增】更新按钮高亮
    updateButtonStyles();

    updateView();
}

void ImageViewWidget::updateView()
{
    if (m_originalPixmap.isNull()) return;

    QRectF viewRect = m_view->viewport()->rect();
    // 【修复】如果视图区域无效（如初始化时），延迟执行
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
            // 【关键修复】适配高 DPI 屏幕
            // 获取当前控件所在的屏幕 DPR。此时由于我们已经捕获了 ScreenChange 事件，
            // 这里获取的值应该是准确的最新值。
            qreal screenDpr = m_view->devicePixelRatioF();

            // 获取图片的 DPR。通常 QImage 加载后为 1.0，除非特别设置。
            qreal imageDpr = m_originalPixmap.devicePixelRatio();

            // 计算缩放因子：目的是让 1 个图片像素对应 1 个物理像素
            // 逻辑坐标大小 = 图片像素 / 屏幕DPR
            qreal factor = imageDpr / screenDpr;

            // 应用缩放
            m_view->scale(factor, factor);
            break;
        }
    }

    // 将图片居中显示
    if (m_pixmapItem) {
        m_view->centerOn(m_pixmapItem);
    }
}

// 【新增】更新按钮高亮样式的辅助函数
void ImageViewWidget::updateButtonStyles()
{
    QString activeStyle = "background-color: rgba(0, 120, 215, 150); color: white; border: none; border-radius: 3px; padding: 2px;";
    QString inactiveStyle = "background-color: rgba(100, 100, 100, 70); color: white; border: none; border-radius: 3px; padding: 2px;";

    // 安全检查
    if (!m_fitToViewBtn) return;

    m_fitToViewBtn->setStyleSheet(m_currentMode == FitToView ? activeStyle : inactiveStyle);
    m_fitToWidthBtn->setStyleSheet(m_currentMode == FitToWidth ? activeStyle : inactiveStyle);
    m_fitToHeightBtn->setStyleSheet(m_currentMode == FitToHeight ? activeStyle : inactiveStyle);
    m_originalSizeBtn->setStyleSheet(m_currentMode == OriginalSize ? activeStyle : inactiveStyle);
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
