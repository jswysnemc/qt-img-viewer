#include "imageview.h"

#include <QContextMenuEvent>
#include <QEvent>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QImage>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QScrollBar>
#include <QStyleOptionGraphicsItem>
#include <QThread>
#include <QWheelEvent>
#include <QWindow>
#include <QtConcurrent>

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace {
constexpr double kZoomStep = 1.18;
constexpr double kMinZoom = 0.05;
constexpr double kMaxZoom = 32.0;
constexpr qsizetype kMaxViewportRenderPixels = 50'000'000;
constexpr double kSharpKernelRadius = 2.5;
constexpr int kMinRowsPerThread = 64;

struct AxisSample {
    int index = 0;
    double weight = 0.0;
};

struct AxisTable {
    std::vector<std::vector<AxisSample>> samples;
    int first = 0;
    int last = -1;
};

QSize scaledSize(const QSize &size, double factor)
{
    return {qMax(1, qRound(size.width() * factor)), qMax(1, qRound(size.height() * factor))};
}

double sharpKernel(double distance)
{
    const double x = std::abs(distance);
    if (x > kSharpKernelRadius) {
        return 0.0;
    }
    if (x <= 0.5) {
        return 17.0 / 16.0 - 7.0 / 4.0 * x * x;
    }
    if (x <= 1.5) {
        return x * x - 11.0 / 4.0 * x + 7.0 / 4.0;
    }
    return -1.0 / 8.0 * x * x + 5.0 / 8.0 * x - 25.0 / 32.0;
}

AxisTable buildAxisTable(int inputSize, int outputSize, double sourceStart, double scale)
{
    AxisTable table;
    table.samples.resize(outputSize);
    table.first = inputSize;

    const double filterScale = std::min(scale, 1.0);
    const double radius = kSharpKernelRadius / filterScale;

    for (int output = 0; output < outputSize; ++output) {
        const double center = sourceStart + (double(output) + 0.5) / scale - 0.5;
        const int first = std::max(0, int(std::floor(center - radius)));
        const int last = std::min(inputSize - 1, int(std::ceil(center + radius)));

        double sum = 0.0;
        std::vector<AxisSample> samples;
        samples.reserve(std::max(0, last - first + 1));
        for (int input = first; input <= last; ++input) {
            const double weight = sharpKernel((double(input) - center) * filterScale);
            if (weight == 0.0) {
                continue;
            }
            samples.push_back({input, weight});
            sum += weight;
        }

        if (samples.empty() || qFuzzyIsNull(sum)) {
            const int nearest = std::clamp(int(std::round(center)), 0, inputSize - 1);
            samples.push_back({nearest, 1.0});
            table.first = std::min(table.first, nearest);
            table.last = std::max(table.last, nearest);
        } else {
            for (AxisSample &sample : samples) {
                sample.weight /= sum;
                table.first = std::min(table.first, sample.index);
                table.last = std::max(table.last, sample.index);
            }
        }

        table.samples[output] = std::move(samples);
    }

    if (table.first > table.last) {
        table.first = 0;
        table.last = 0;
    }

    return table;
}

int byteFromWeightedSum(double value)
{
    return std::clamp(int(std::lround(value)), 0, 255);
}

QRgb premultipliedPixel(double red, double green, double blue, double alpha)
{
    const int a = byteFromWeightedSum(alpha);
    const int r = std::min(byteFromWeightedSum(red), a);
    const int g = std::min(byteFromWeightedSum(green), a);
    const int b = std::min(byteFromWeightedSum(blue), a);
    return qRgba(r, g, b, a);
}

template <typename Function>
void forRowRanges(int rowCount, Function function)
{
    if (rowCount <= 0) {
        return;
    }

    const int idealThreads = std::max(1, QThread::idealThreadCount());
    const int threadCount = std::clamp(rowCount / kMinRowsPerThread, 1, idealThreads);
    if (threadCount == 1) {
        function(0, rowCount);
        return;
    }

    std::vector<std::pair<int, int>> ranges;
    ranges.reserve(threadCount);
    const int rowsPerThread = rowCount / threadCount;
    int begin = 0;
    for (int i = 0; i < threadCount; ++i) {
        const int end = i == threadCount - 1 ? rowCount : begin + rowsPerThread;
        ranges.push_back({begin, end});
        begin = end;
    }

    QtConcurrent::blockingMap(ranges, [&function](const std::pair<int, int> &range) {
        function(range.first, range.second);
    });
}

QImage renderSharpViewport(const QImage &source, const QRectF &sourceRect, const QSize &targetSize)
{
    if (source.isNull() || sourceRect.isEmpty() || targetSize.isEmpty()) {
        return {};
    }

    const double scaleX = double(targetSize.width()) / sourceRect.width();
    const double scaleY = double(targetSize.height()) / sourceRect.height();
    const AxisTable xTable = buildAxisTable(source.width(), targetSize.width(), sourceRect.left(), scaleX);
    const AxisTable yTable = buildAxisTable(source.height(), targetSize.height(), sourceRect.top(), scaleY);

    const int intermediateHeight = yTable.last - yTable.first + 1;
    QImage horizontal(targetSize.width(), intermediateHeight, QImage::Format_ARGB32_Premultiplied);
    const uchar *sourceBits = source.constBits();
    const int sourceStride = source.bytesPerLine();
    uchar *horizontalBits = horizontal.bits();
    const int horizontalStride = horizontal.bytesPerLine();

    forRowRanges(intermediateHeight, [&](int begin, int end) {
        for (int row = begin; row < end; ++row) {
            const int y = yTable.first + row;
            const auto *sourceLine = reinterpret_cast<const QRgb *>(sourceBits + y * sourceStride);
            auto *targetLine = reinterpret_cast<QRgb *>(horizontalBits + row * horizontalStride);
            for (int x = 0; x < targetSize.width(); ++x) {
                double a = 0.0;
                double r = 0.0;
                double g = 0.0;
                double b = 0.0;
                for (const AxisSample &sample : xTable.samples[x]) {
                    const QRgb pixel = sourceLine[sample.index];
                    a += sample.weight * double(qAlpha(pixel));
                    r += sample.weight * double(qRed(pixel));
                    g += sample.weight * double(qGreen(pixel));
                    b += sample.weight * double(qBlue(pixel));
                }
                targetLine[x] = premultipliedPixel(r, g, b, a);
            }
        }
    });

    QImage target(targetSize, QImage::Format_ARGB32_Premultiplied);
    const uchar *horizontalReadBits = horizontal.constBits();
    const int horizontalReadStride = horizontal.bytesPerLine();
    uchar *targetBits = target.bits();
    const int targetStride = target.bytesPerLine();

    forRowRanges(targetSize.height(), [&](int begin, int end) {
        for (int y = begin; y < end; ++y) {
            auto *targetLine = reinterpret_cast<QRgb *>(targetBits + y * targetStride);
            for (int x = 0; x < targetSize.width(); ++x) {
                double a = 0.0;
                double r = 0.0;
                double g = 0.0;
                double b = 0.0;
                for (const AxisSample &sample : yTable.samples[y]) {
                    const auto *sourceLine = reinterpret_cast<const QRgb *>(horizontalReadBits + (sample.index - yTable.first) * horizontalReadStride);
                    const QRgb pixel = sourceLine[x];
                    a += sample.weight * double(qAlpha(pixel));
                    r += sample.weight * double(qRed(pixel));
                    g += sample.weight * double(qGreen(pixel));
                    b += sample.weight * double(qBlue(pixel));
                }
                targetLine[x] = premultipliedPixel(r, g, b, a);
            }
        }
    });

    return target;
}

class SharpPixmapItem : public QGraphicsPixmapItem
{
public:
    SharpPixmapItem()
    {
        setFlag(QGraphicsItem::ItemUsesExtendedStyleOption, true);
        setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
    }

    void setSourcePixmap(const QPixmap &pixmap)
    {
        m_sourceImage = pixmap.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
        setPixmap(pixmap);
    }

    void setDeviceScale(double scale)
    {
        m_deviceScale = scale;
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override
    {
        if (m_sourceImage.isNull() || qFuzzyCompare(m_deviceScale, 1.0)) {
            QGraphicsPixmapItem::paint(painter, option, widget);
            return;
        }

        const QRectF sourceRect = option->exposedRect.intersected(boundingRect());
        if (sourceRect.isEmpty()) {
            return;
        }

        const double dpr = widget ? widget->devicePixelRatioF() : painter->device()->devicePixelRatioF();
        const QRectF targetRect = painter->worldTransform().mapRect(sourceRect);
        const QSize targetSize(qMax(1, qRound(targetRect.width() * dpr)), qMax(1, qRound(targetRect.height() * dpr)));
        const qsizetype targetPixels = qsizetype(targetSize.width()) * qsizetype(targetSize.height());
        if (targetPixels > kMaxViewportRenderPixels) {
            QGraphicsPixmapItem::paint(painter, option, widget);
            return;
        }

        QImage rendered = renderSharpViewport(m_sourceImage, sourceRect, targetSize);
        if (rendered.isNull()) {
            QGraphicsPixmapItem::paint(painter, option, widget);
            return;
        }

        rendered.setDevicePixelRatio(dpr);
        const QRectF drawRect(targetRect.topLeft(), QSizeF(double(targetSize.width()) / dpr, double(targetSize.height()) / dpr));

        painter->save();
        painter->setWorldTransform(QTransform());
        painter->setClipRect(drawRect);
        painter->drawImage(drawRect, rendered);
        painter->restore();
    }

private:
    QImage m_sourceImage;
    double m_deviceScale = 1.0;
};
}

ImageView::ImageView(QWidget *parent)
    : QGraphicsView(parent)
    , m_scene(new QGraphicsScene(this))
    , m_pixmapItem(new SharpPixmapItem())
{
    m_scene->addItem(m_pixmapItem);
    setScene(m_scene);

    setAlignment(Qt::AlignCenter);
    setBackgroundBrush(QColor(239, 236, 229));
    setDragMode(QGraphicsView::ScrollHandDrag);
    setFrameShape(QFrame::NoFrame);
    setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing, true);
    setOptimizationFlag(QGraphicsView::DontSavePainterState, true);
    setRenderHint(QPainter::Antialiasing, false);
    setRenderHint(QPainter::SmoothPixmapTransform, false);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
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
    m_displayPixmapCacheKey = 0;
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

    const double dpr = displayDevicePixelRatio();
    const double xFactor = (double(viewSize.width()) * dpr) / double(sourceSize.width());
    const double yFactor = (double(viewSize.height()) * dpr) / double(sourceSize.height());
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

bool ImageView::event(QEvent *event)
{
    const bool handled = QGraphicsView::event(event);
    if (event->type() == QEvent::DevicePixelRatioChange && hasImage()) {
        if (m_fitMode) {
            fitToWindow();
        } else {
            updateDisplayedPixmap();
        }
    }
    return handled;
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

double ImageView::displayDevicePixelRatio() const
{
    return std::max(1.0, viewport() ? viewport()->devicePixelRatioF() : devicePixelRatioF());
}

double ImageView::sceneScaleFactor() const
{
    return m_zoomFactor / displayDevicePixelRatio();
}

void ImageView::setZoomFactor(double factor)
{
    const double oldSceneScale = sceneScaleFactor();
    const bool hasCenter = oldSceneScale > 0.0;
    QPointF imageCenter;
    if (hasCenter) {
        imageCenter = mapToScene(viewport()->rect().center()) / oldSceneScale;
    }

    m_zoomFactor = factor;
    updateDisplayedPixmap();
    if (hasCenter) {
        centerOn(imageCenter * sceneScaleFactor());
    }
    emit zoomChanged(m_zoomFactor);
}

QSizeF ImageView::targetLogicalSize() const
{
    if (!hasImage()) {
        return {};
    }

    const QSize targetSize = scaledSize(m_sourcePixmap.size(), m_zoomFactor);
    const double dpr = displayDevicePixelRatio();
    return QSizeF(double(targetSize.width()) / dpr, double(targetSize.height()) / dpr);
}

void ImageView::showSourcePixmap()
{
    auto *sharpItem = static_cast<SharpPixmapItem *>(m_pixmapItem);
    if (m_displayPixmapCacheKey != m_sourcePixmap.cacheKey()) {
        sharpItem->setSourcePixmap(m_sourcePixmap);
        m_displayPixmapCacheKey = m_sourcePixmap.cacheKey();
    }
    sharpItem->setDeviceScale(m_zoomFactor);
    m_pixmapItem->setTransformationMode(qFuzzyCompare(m_zoomFactor, 1.0) ? Qt::FastTransformation : Qt::SmoothTransformation);
    m_pixmapItem->setScale(sceneScaleFactor());
}

void ImageView::updateDisplayedPixmap()
{
    if (!hasImage()) {
        m_pixmapItem->setScale(1.0);
        m_pixmapItem->setPixmap({});
        m_displayPixmapCacheKey = 0;
        updateSceneRect();
        return;
    }

    showSourcePixmap();
    updateSceneRect();
}

void ImageView::updateSceneRect()
{
    if (!hasImage()) {
        m_scene->setSceneRect(rect());
        return;
    }

    m_pixmapItem->setOffset(0, 0);
    m_scene->setSceneRect(QRectF(QPointF(0, 0), targetLogicalSize()));
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
    const double oldSceneScale = sceneScaleFactor();
    const QPointF imagePoint = mapToScene(position) / oldSceneScale;
    const QPointF cursorOffset = QPointF(viewportCenter - position);

    m_fitMode = false;
    m_zoomFactor = target;
    updateDisplayedPixmap();
    centerOn(imagePoint * sceneScaleFactor() + cursorOffset);
    emit zoomChanged(m_zoomFactor);
}
