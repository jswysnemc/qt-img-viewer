#pragma once

#include <QGraphicsView>
#include <QPoint>

class QGraphicsPixmapItem;
class QGraphicsScene;

class ImageView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit ImageView(QWidget *parent = nullptr);

    bool hasImage() const;
    QSize imageSize() const;
    QPixmap pixmap() const;
    double imageOpacity() const;
    double zoomFactor() const;
    QString imagePath() const;

public slots:
    void clearImage();
    void fitToWindow();
    void resetZoom();
    void setImage(const QPixmap &pixmap, const QString &path);
    void setImageFrame(const QPixmap &pixmap);
    void setImageOpacity(double opacity);
    void setResizeFitEnabled(bool enabled);
    void setWindowDragEnabled(bool enabled);
    void zoomIn();
    void zoomOut();

signals:
    void contextMenuRequested(const QPoint &globalPos);
    void zoomChanged(double factor);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void applyScale(double factor);
    void setZoomFactor(double factor);
    void updateDisplayedPixmap();
    void updateSceneRect();
    void zoomAt(const QPoint &position, double factor);

    QGraphicsScene *m_scene = nullptr;
    QGraphicsPixmapItem *m_pixmapItem = nullptr;
    QPixmap m_sourcePixmap;
    QString m_imagePath;
    bool m_windowDragEnabled = false;
    bool m_windowDragging = false;
    QPoint m_windowDragOffset;
    bool m_middlePanning = false;
    QPoint m_lastPanPosition;
    double m_zoomFactor = 1.0;
    bool m_fitMode = true;
};
