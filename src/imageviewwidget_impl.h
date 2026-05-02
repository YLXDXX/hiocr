#ifndef IMAGEVIEWWIDGET_IMPL_H
#define IMAGEVIEWWIDGET_IMPL_H

#include "imageviewwidget.h"

// --- Implementation ---

inline bool ImageViewWidget::event(QEvent* event)
{
    if (event->type() == QEvent::ScreenChangeInternal ||
        event->type() == QEvent::DevicePixelRatioChange) {

        if (hasImage() && !m_manualZoomMode) {
            updateView();
        }
        }
        return QWidget::event(event);
}

inline ImageViewWidget::ImageViewWidget(QWidget* parent) : QWidget(parent), m_manualZoomMode(false)
{
    setupUi();

    int modeIndex = SettingsManager::instance()->imageViewMode();
    if (modeIndex >= 0 && modeIndex <= 3) {
        m_currentMode = static_cast<ViewMode>(modeIndex);
    } else {
        m_currentMode = FitToView;
    }

    updateButtonStyles();

    m_view->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_view->setInteractive(false);
}

inline void ImageViewWidget::setupUi()
{
    QVBoxLayout* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    QLabel* hint = new QLabel("提示：可按 Ctrl+V 粘贴剪贴板图片", this);
    hint->setStyleSheet("color: gray; font-size: 12px;");
    outerLayout->addWidget(hint);

    QWidget* viewContainer = new QWidget(this);
    QGridLayout* gridLayout = new QGridLayout(viewContainer);
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setSpacing(0);

    m_view = new QGraphicsView(this);
    m_scene = new QGraphicsScene(this);
    m_view->setScene(m_scene);

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

    m_pixmapItem = nullptr;

    m_buttonContainer = new QWidget(this);
    m_buttonContainer->setStyleSheet("background-color: rgba(200, 200, 200, 80); border-radius: 4px;");
    QHBoxLayout* btnLayout = new QHBoxLayout(m_buttonContainer);
    btnLayout->setContentsMargins(4, 4, 4, 4);
    btnLayout->setSpacing(4);

    m_fitToViewBtn = new QPushButton("整页");
    m_fitToWidthBtn = new QPushButton("同宽");
    m_fitToHeightBtn = new QPushButton("同高");
    m_originalSizeBtn = new QPushButton("原大", this);

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

    gridLayout->addWidget(m_view, 0, 0);
    gridLayout->addWidget(m_buttonContainer, 0, 0, Qt::AlignRight | Qt::AlignBottom);
    gridLayout->setColumnStretch(0, 1);
    gridLayout->setRowStretch(0, 1);

    outerLayout->addWidget(viewContainer);
}

inline void ImageViewWidget::setImage(const QImage& image)
{
    if (image.isNull()) {
        QMessageBox::warning(this, "错误", "图片无效");
        return;
    }

    m_originalImage = image;

    m_originalPixmap = QPixmap::fromImage(image);

    m_manualZoomMode = false;
    m_scene->clear();
    m_pixmapItem = m_scene->addPixmap(m_originalPixmap);
    m_scene->setSceneRect(m_originalPixmap.rect());
    updateBase64FromImage(image);
    updateView();
    emit imageChanged();
    m_view->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    m_view->setInteractive(true);
}

inline void ImageViewWidget::setImage(const QString& path)
{
    QImage image = ImageProcessor::loadImage(path);
    if (image.isNull()) {
        QMessageBox::warning(this, "错误", "无法加载图片: " + path);
        return;
    }
    setImage(image);
}

inline void ImageViewWidget::clear()
{
    m_scene->clear();
    m_pixmapItem = nullptr;
    m_originalPixmap = QPixmap();

    m_originalImage = QImage();

    m_currentBase64.clear();
    emit imageChanged();

    m_view->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_view->setInteractive(false);
}

inline QImage ImageViewWidget::currentImage() const
{
    return m_originalImage;
}

inline void ImageViewWidget::setViewMode(ViewMode mode)
{
    if (m_currentMode == mode && !m_manualZoomMode) return;

    m_currentMode = mode;
    m_manualZoomMode = false;

    SettingsManager::instance()->setImageViewMode(static_cast<int>(mode));

    updateButtonStyles();

    updateView();
}

inline void ImageViewWidget::updateView()
{
    if (m_originalPixmap.isNull()) return;

    QRectF viewRect = m_view->viewport()->rect();
    if (viewRect.isEmpty()) {
        QTimer::singleShot(0, this, &ImageViewWidget::updateView);
        return;
    }

    if (m_manualZoomMode) return;

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
            qreal screenDpr = m_view->devicePixelRatioF();

            qreal imageDpr = m_originalPixmap.devicePixelRatio();

            qreal factor = imageDpr / screenDpr;

            m_view->scale(factor, factor);
            break;
        }
    }

    if (m_pixmapItem) {
        m_view->centerOn(m_pixmapItem);
    }
}

inline void ImageViewWidget::updateButtonStyles()
{
    QString activeStyle = "background-color: rgba(0, 120, 215, 150); color: white; border: none; border-radius: 3px; padding: 2px;";
    QString inactiveStyle = "background-color: rgba(100, 100, 100, 70); color: white; border: none; border-radius: 3px; padding: 2px;";

    if (!m_fitToViewBtn) return;

    m_fitToViewBtn->setStyleSheet(m_currentMode == FitToView ? activeStyle : inactiveStyle);
    m_fitToWidthBtn->setStyleSheet(m_currentMode == FitToWidth ? activeStyle : inactiveStyle);
    m_fitToHeightBtn->setStyleSheet(m_currentMode == FitToHeight ? activeStyle : inactiveStyle);
    m_originalSizeBtn->setStyleSheet(m_currentMode == OriginalSize ? activeStyle : inactiveStyle);
}

inline void ImageViewWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    if (hasImage()) {
        updateView();
    }
}

inline void ImageViewWidget::updateBase64FromImage(const QImage& image)
{
    m_currentBase64 = ImageProcessor::imageToBase64(image);
}

inline void ImageViewWidget::mousePressEvent(QMouseEvent* event)
{
    if (!hasImage() && event->button() == Qt::LeftButton) {
        openImageFile();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}


inline void ImageViewWidget::wheelEvent(QWheelEvent* event)
{
    if (hasImage() && (event->modifiers() & Qt::ControlModifier)) {
        m_manualZoomMode = true;

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

inline void ImageViewWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (hasImage() && !m_manualZoomMode) {
        updateView();
    }
}

inline void ImageViewWidget::openImageFile()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("选择图片文件"),
                                                    QString(),
                                                    tr("Images (*.png *.jpg *.jpeg *.bmp *.gif)"));
    if (!fileName.isEmpty()) {
        setImage(fileName);
    }
}

inline void ImageViewWidget::onFitToViewClicked()
{
    setViewMode(FitToView);
}

inline void ImageViewWidget::onFitToWidthClicked()
{
    setViewMode(FitToWidth);
}

inline void ImageViewWidget::onFitToHeightClicked()
{
    setViewMode(FitToHeight);
}

inline void ImageViewWidget::onOriginalSizeClicked()
{
    setViewMode(OriginalSize);
}

#endif // IMAGEVIEWWIDGET_IMPL_H
