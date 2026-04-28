#include "imageview.h"

#include <QContextMenuEvent>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QScrollBar>
#include <QWheelEvent>
#include <QWindow>

#include <algorithm>

namespace {
constexpr double kZoomStep = 1.18;
constexpr double kMinZoom = 0.05;
constexpr double kMaxZoom = 32.0;
constexpr qsizetype kMaxCachedPixels = 120'000'000;

QSize scaledSize(const QSize &size, double factor)
{
    return {qMax(1, qRound(size.width() * factor)), qMax(1, qRound(size.height() * factor))};
}
}

ImageView::ImageView(QWidget *parent)
    : QGraphicsView(parent)
    , m_scene(new QGraphicsScene(this))
    , m_pixmapItem(new QGraphicsPixmapItem())
{
    m_scene->addItem(m_pixmapItem);
    setScene(m_scene);

    setAlignment(Qt::AlignCenter);
    setBackgroundBrush(QColor(239, 236, 229));
    setDragMode(QGraphicsView::ScrollHandDrag);
    setFrameShape(QFrame::NoFrame);
    setRenderHint(QPainter::Antialiasing, false);
    setRenderHint(QPainter::SmoothPixmapTransform, false);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
}

bool ImageView::hasImage() const
{
    return !m_sourcePixmap.isNull();
}

QSize ImageView::imageSize() const
{
    return m_sourcePixmap.size();
}

QPixmap ImageView::pixmap() const
{
    return m_sourcePixmap;
}

double ImageView::imageOpacity() const
{
    return m_pixmapItem->opacity();
}

double ImageView::zoomFactor() const
{
    return m_zoomFactor;
}

QString ImageView::imagePath() const
{
    return m_imagePath;
}

void ImageView::clearImage()
{
    m_sourcePixmap = {};
    m_pixmapItem->setPixmap({});
    m_imagePath.clear();
    m_zoomFactor = 1.0;
    m_fitMode = true;
    resetTransform();
    updateSceneRect();
    emit zoomChanged(m_zoomFactor);
}

void ImageView::fitToWindow()
{
    if (!hasImage()) {
        return;
    }

    m_fitMode = true;
    const QSize sourceSize = m_sourcePixmap.size();
    const QSize viewSize = viewport()->size();
    if (!sourceSize.isValid() || !viewSize.isValid()) {
        return;
    }

    const double xFactor = double(viewSize.width()) / double(sourceSize.width());
    const double yFactor = double(viewSize.height()) / double(sourceSize.height());
    m_zoomFactor = std::clamp(std::min(xFactor, yFactor), kMinZoom, kMaxZoom);
    updateDisplayedPixmap();
    centerOn(m_pixmapItem);
    emit zoomChanged(m_zoomFactor);
}

void ImageView::resetZoom()
{
    if (!hasImage()) {
        return;
    }

    m_fitMode = false;
    resetTransform();
    setZoomFactor(1.0);
    centerOn(m_pixmapItem);
}

void ImageView::setImage(const QPixmap &pixmap, const QString &path)
{
    const double opacity = m_pixmapItem->opacity();
    m_sourcePixmap = pixmap;
    m_pixmapItem->setOpacity(opacity);
    m_imagePath = path;
    fitToWindow();
}

void ImageView::setImageFrame(const QPixmap &pixmap)
{
    const bool sizeChanged = pixmap.size() != m_sourcePixmap.size();
    const double opacity = m_pixmapItem->opacity();
    m_sourcePixmap = pixmap;
    m_pixmapItem->setOpacity(opacity);
    if (!sizeChanged) {
        updateDisplayedPixmap();
        viewport()->update();
        return;
    }

    if (m_fitMode) {
        fitToWindow();
    } else {
        updateDisplayedPixmap();
    }
}

void ImageView::setImageOpacity(double opacity)
{
    m_pixmapItem->setOpacity(std::clamp(opacity, 0.2, 1.0));
}

void ImageView::setResizeFitEnabled(bool enabled)
{
    m_fitMode = enabled;
}

void ImageView::setWindowDragEnabled(bool enabled)
{
    m_windowDragEnabled = enabled;
    m_windowDragging = false;
    m_windowDragOffset = {};
    m_middlePanning = false;
    m_lastPanPosition = {};
    setDragMode(enabled ? QGraphicsView::NoDrag : QGraphicsView::ScrollHandDrag);
}

void ImageView::zoomIn()
{
    zoomAt(viewport()->rect().center(), kZoomStep);
}

void ImageView::zoomOut()
{
    zoomAt(viewport()->rect().center(), 1.0 / kZoomStep);
}

void ImageView::contextMenuEvent(QContextMenuEvent *event)
{
    if (hasImage()) {
        emit contextMenuRequested(event->globalPos());
        event->accept();
        return;
    }

    QGraphicsView::contextMenuEvent(event);
}

void ImageView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_middlePanning) {
        const QPoint position = event->position().toPoint();
        const QPoint delta = position - m_lastPanPosition;
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        m_lastPanPosition = position;
        event->accept();
        return;
    }

    if (m_windowDragging && m_windowDragEnabled) {
        window()->move(event->globalPosition().toPoint() - m_windowDragOffset);
        event->accept();
        return;
    }

    QGraphicsView::mouseMoveEvent(event);
}

void ImageView::mousePressEvent(QMouseEvent *event)
{
    if (hasImage() && event->button() == Qt::MiddleButton) {
        m_middlePanning = true;
        m_lastPanPosition = event->position().toPoint();
        viewport()->setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    if (m_windowDragEnabled && event->button() == Qt::LeftButton) {
        QWidget *host = window();
        if (QWindow *windowHandle = host->windowHandle()) {
            if (windowHandle->startSystemMove()) {
                event->accept();
                return;
            }
        }

        m_windowDragging = true;
        m_windowDragOffset = event->globalPosition().toPoint() - host->frameGeometry().topLeft();
        event->accept();
        return;
    }

    QGraphicsView::mousePressEvent(event);
}

void ImageView::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_middlePanning && event->button() == Qt::MiddleButton) {
        m_middlePanning = false;
        m_lastPanPosition = {};
        viewport()->unsetCursor();
        event->accept();
        return;
    }

    if (m_windowDragging && event->button() == Qt::LeftButton) {
        m_windowDragging = false;
        m_windowDragOffset = {};
        event->accept();
        return;
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void ImageView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    if (m_fitMode) {
        fitToWindow();
    }
}

void ImageView::wheelEvent(QWheelEvent *event)
{
    if (!hasImage()) {
        QGraphicsView::wheelEvent(event);
        return;
    }

    const int delta = event->angleDelta().y();
    if (delta == 0) {
        QGraphicsView::wheelEvent(event);
        return;
    }

    zoomAt(event->position().toPoint(), delta > 0 ? kZoomStep : 1.0 / kZoomStep);
    event->accept();
}

void ImageView::applyScale(double factor)
{
    if (!hasImage()) {
        return;
    }

    const double target = std::clamp(m_zoomFactor * factor, kMinZoom, kMaxZoom);
    const double effectiveFactor = target / m_zoomFactor;
    if (qFuzzyCompare(effectiveFactor, 1.0)) {
        return;
    }

    m_fitMode = false;
    zoomAt(viewport()->rect().center(), effectiveFactor);
}

void ImageView::setZoomFactor(double factor)
{
    QPointF imageCenter;
    if (m_zoomFactor > 0.0) {
        imageCenter = mapToScene(viewport()->rect().center()) / m_zoomFactor;
    }

    m_zoomFactor = factor;
    updateDisplayedPixmap();
    if (!imageCenter.isNull()) {
        centerOn(imageCenter * m_zoomFactor);
    }
    emit zoomChanged(m_zoomFactor);
}

void ImageView::updateDisplayedPixmap()
{
    if (!hasImage()) {
        m_pixmapItem->setScale(1.0);
        m_pixmapItem->setPixmap({});
        updateSceneRect();
        return;
    }

    const QSize targetSize = scaledSize(m_sourcePixmap.size(), m_zoomFactor);
    const qsizetype targetPixels = qsizetype(targetSize.width()) * qsizetype(targetSize.height());
    const bool useCache = targetSize != m_sourcePixmap.size() && targetPixels <= kMaxCachedPixels;

    if (useCache) {
        const QImage scaled = m_sourcePixmap.toImage().scaled(targetSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        m_pixmapItem->setPixmap(QPixmap::fromImage(scaled));
        m_pixmapItem->setScale(1.0);
    } else {
        m_pixmapItem->setPixmap(m_sourcePixmap);
        m_pixmapItem->setScale(m_zoomFactor);
    }

    updateSceneRect();
}

void ImageView::updateSceneRect()
{
    if (!hasImage()) {
        m_scene->setSceneRect(rect());
        return;
    }

    m_pixmapItem->setOffset(0, 0);
    m_scene->setSceneRect(QRectF(QPointF(0, 0), QSizeF(scaledSize(m_sourcePixmap.size(), m_zoomFactor))));
}

void ImageView::zoomAt(const QPoint &position, double factor)
{
    if (!hasImage()) {
        return;
    }

    const double target = std::clamp(m_zoomFactor * factor, kMinZoom, kMaxZoom);
    const double effectiveFactor = target / m_zoomFactor;
    if (qFuzzyCompare(effectiveFactor, 1.0)) {
        return;
    }

    const QPoint viewportCenter = viewport()->rect().center();
    const QPointF imagePoint = mapToScene(position) / m_zoomFactor;
    const QPointF cursorOffset = QPointF(viewportCenter - position);

    m_fitMode = false;
    m_zoomFactor = target;
    updateDisplayedPixmap();
    centerOn(imagePoint * m_zoomFactor + cursorOffset);
    emit zoomChanged(m_zoomFactor);
}
