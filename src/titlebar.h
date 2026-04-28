#pragma once

#include <QPoint>
#include <QWidget>

#include "uiicons.h"

class QAction;
class QLabel;
class QHBoxLayout;
class QToolButton;

class TitleBar : public QWidget
{
    Q_OBJECT

public:
    explicit TitleBar(QWidget *parent = nullptr);

    void addActionButton(QAction *action);
    void setTitle(const QString &title);

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QToolButton *createWindowButton(UiIcon icon, const QString &tooltip);
    QWidget *hostWindow() const;
    void toggleMaximized();

    QLabel *m_titleLabel = nullptr;
    QHBoxLayout *m_layout = nullptr;
    QToolButton *m_maximizeButton = nullptr;
    bool m_dragging = false;
    QPoint m_dragOffset;
};
