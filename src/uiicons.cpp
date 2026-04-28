#include "uiicons.h"

#include <QPainter>
#include <QPainterPath>
#include <QPixmap>

namespace {
constexpr int kIconSize = 24;

QColor iconColor(QIcon::Mode mode)
{
    switch (mode) {
    case QIcon::Disabled:
        return QColor(166, 172, 182);
    case QIcon::Selected:
        return QColor(255, 255, 255);
    default:
        return QColor(42, 53, 67);
    }
}

QPixmap drawIconPixmap(UiIcon icon, QIcon::Mode mode)
{
    QPixmap pixmap(kIconSize, kIconSize);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QColor color = iconColor(mode);
    QPen pen(color, 1.9, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    switch (icon) {
    case UiIcon::OpenFile:
        painter.drawRoundedRect(QRectF(6.5, 3.5, 10.5, 17.0), 1.6, 1.6);
        painter.drawLine(QPointF(11.5, 3.5), QPointF(17.0, 9.0));
        painter.drawLine(QPointF(11.5, 3.5), QPointF(11.5, 9.0));
        painter.drawLine(QPointF(11.5, 9.0), QPointF(17.0, 9.0));
        painter.drawLine(QPointF(8.8, 13.0), QPointF(14.6, 13.0));
        painter.drawLine(QPointF(8.8, 16.0), QPointF(14.6, 16.0));
        break;
    case UiIcon::OpenFolder:
        painter.drawPath([] {
            QPainterPath path;
            path.moveTo(3.5, 7.5);
            path.lineTo(8.4, 7.5);
            path.lineTo(10.4, 10.0);
            path.lineTo(20.5, 10.0);
            path.lineTo(20.5, 18.2);
            path.quadTo(20.5, 20.5, 18.2, 20.5);
            path.lineTo(5.8, 20.5);
            path.quadTo(3.5, 20.5, 3.5, 18.2);
            path.closeSubpath();
            return path;
        }());
        painter.drawLine(QPointF(3.8, 10.0), QPointF(20.0, 10.0));
        break;
    case UiIcon::Previous:
        painter.drawLine(QPointF(15.5, 5.0), QPointF(8.5, 12.0));
        painter.drawLine(QPointF(8.5, 12.0), QPointF(15.5, 19.0));
        break;
    case UiIcon::Next:
        painter.drawLine(QPointF(8.5, 5.0), QPointF(15.5, 12.0));
        painter.drawLine(QPointF(15.5, 12.0), QPointF(8.5, 19.0));
        break;
    case UiIcon::ZoomOut:
        painter.drawEllipse(QRectF(4.5, 4.5, 10.5, 10.5));
        painter.drawLine(QPointF(13.0, 13.0), QPointF(19.0, 19.0));
        painter.drawLine(QPointF(7.5, 9.8), QPointF(12.0, 9.8));
        break;
    case UiIcon::ZoomIn:
        painter.drawEllipse(QRectF(4.5, 4.5, 10.5, 10.5));
        painter.drawLine(QPointF(13.0, 13.0), QPointF(19.0, 19.0));
        painter.drawLine(QPointF(7.5, 9.8), QPointF(12.0, 9.8));
        painter.drawLine(QPointF(9.75, 7.5), QPointF(9.75, 12.0));
        break;
    case UiIcon::Fit:
        painter.drawRect(QRectF(5.0, 5.0, 14.0, 14.0));
        painter.drawLine(QPointF(8.0, 8.0), QPointF(11.0, 8.0));
        painter.drawLine(QPointF(8.0, 8.0), QPointF(8.0, 11.0));
        painter.drawLine(QPointF(16.0, 8.0), QPointF(13.0, 8.0));
        painter.drawLine(QPointF(16.0, 8.0), QPointF(16.0, 11.0));
        painter.drawLine(QPointF(8.0, 16.0), QPointF(11.0, 16.0));
        painter.drawLine(QPointF(8.0, 16.0), QPointF(8.0, 13.0));
        painter.drawLine(QPointF(16.0, 16.0), QPointF(13.0, 16.0));
        painter.drawLine(QPointF(16.0, 16.0), QPointF(16.0, 13.0));
        break;
    case UiIcon::ActualSize:
        painter.drawRoundedRect(QRectF(5.0, 5.0, 14.0, 14.0), 1.6, 1.6);
        painter.drawText(QRectF(4.0, 5.0, 16.0, 14.0), Qt::AlignCenter, QStringLiteral("1:1"));
        break;
    case UiIcon::RotateLeft:
        painter.drawArc(QRectF(5.5, 5.5, 13.0, 13.0), 30 * 16, 260 * 16);
        painter.drawLine(QPointF(6.5, 8.0), QPointF(6.8, 3.8));
        painter.drawLine(QPointF(6.5, 8.0), QPointF(10.5, 7.1));
        break;
    case UiIcon::RotateRight:
        painter.drawArc(QRectF(5.5, 5.5, 13.0, 13.0), -110 * 16, 260 * 16);
        painter.drawLine(QPointF(17.5, 8.0), QPointF(17.2, 3.8));
        painter.drawLine(QPointF(17.5, 8.0), QPointF(13.5, 7.1));
        break;
    case UiIcon::Sidebar:
        painter.drawRoundedRect(QRectF(4.5, 5.0, 15.0, 14.0), 1.8, 1.8);
        painter.drawLine(QPointF(9.5, 5.5), QPointF(9.5, 18.5));
        painter.drawLine(QPointF(6.7, 8.5), QPointF(7.4, 8.5));
        painter.drawLine(QPointF(6.7, 11.8), QPointF(7.4, 11.8));
        painter.drawLine(QPointF(6.7, 15.1), QPointF(7.4, 15.1));
        break;
    case UiIcon::Pin:
        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(QRectF(7.0, 3.5, 10.0, 3.8), 1.5, 1.5);
        painter.drawPath([] {
            QPainterPath path;
            path.moveTo(10.0, 7.2);
            path.lineTo(14.0, 7.2);
            path.lineTo(15.8, 14.0);
            path.lineTo(8.2, 14.0);
            path.closeSubpath();
            return path;
        }());
        painter.setPen(QPen(color, 1.9, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawLine(QPointF(12.0, 14.0), QPointF(12.0, 21.0));
        painter.drawLine(QPointF(9.5, 17.0), QPointF(14.5, 17.0));
        break;
    case UiIcon::Info:
        painter.drawEllipse(QRectF(5.0, 5.0, 14.0, 14.0));
        painter.drawPoint(QPointF(12.0, 8.8));
        painter.drawLine(QPointF(12.0, 11.5), QPointF(12.0, 16.0));
        break;
    case UiIcon::Minimize:
        painter.drawLine(QPointF(7.0, 15.0), QPointF(17.0, 15.0));
        break;
    case UiIcon::Maximize:
        painter.drawRoundedRect(QRectF(6.5, 6.5, 11.0, 11.0), 1.2, 1.2);
        break;
    case UiIcon::Close:
        painter.drawLine(QPointF(7.5, 7.5), QPointF(16.5, 16.5));
        painter.drawLine(QPointF(16.5, 7.5), QPointF(7.5, 16.5));
        break;
    }

    return pixmap;
}
}

QIcon makeUiIcon(UiIcon icon)
{
    QIcon result;
    result.addPixmap(drawIconPixmap(icon, QIcon::Normal), QIcon::Normal);
    result.addPixmap(drawIconPixmap(icon, QIcon::Selected), QIcon::Normal, QIcon::On);
    result.addPixmap(drawIconPixmap(icon, QIcon::Disabled), QIcon::Disabled);
    result.addPixmap(drawIconPixmap(icon, QIcon::Disabled), QIcon::Disabled, QIcon::On);
    result.addPixmap(drawIconPixmap(icon, QIcon::Selected), QIcon::Selected);
    result.addPixmap(drawIconPixmap(icon, QIcon::Selected), QIcon::Selected, QIcon::On);
    return result;
}
