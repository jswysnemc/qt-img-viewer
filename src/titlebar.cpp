#include "titlebar.h"

#include <QAction>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QStyle>
#include <QToolButton>
#include <QWindow>

namespace {
constexpr int kTitleBarHeight = 48;
constexpr int kButtonSize = 36;
constexpr int kIconSize = 20;

QString buttonStyle()
{
    return QStringLiteral(
        "QToolButton {"
        "  border: 0;"
        "  border-radius: 8px;"
        "  min-width: 34px;"
        "  min-height: 34px;"
        "  max-width: 34px;"
        "  max-height: 34px;"
        "  padding: 0;"
        "  background: transparent;"
        "}"
        "QToolButton:hover { background: #ece6dc; }"
        "QToolButton:pressed { background: #ddd5c8; }"
        "QToolButton:checked { background: #2f63d8; }"
        "QToolButton:checked:hover { background: #2453bd; }"
        "QToolButton:disabled { background: transparent; }"
        "QToolButton#closeButton:hover { background: #c3312b; color: white; }");
}
}

TitleBar::TitleBar(QWidget *parent)
    : QWidget(parent)
    , m_titleLabel(new QLabel(this))
    , m_layout(new QHBoxLayout(this))
{
    setFixedHeight(kTitleBarHeight);
    setObjectName(QStringLiteral("titleBar"));
    setAutoFillBackground(true);
    setStyleSheet(QStringLiteral(
        "QWidget#titleBar {"
        "  background: #fbfaf7;"
        "  border-bottom: 1px solid #ddd7cd;"
        "}"
        "QLabel { color: #263241; font-weight: 700; padding-left: 2px; }")
        + buttonStyle());

    m_titleLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_titleLabel->setText(QStringLiteral("Qt Image Viewer"));

    m_layout->setContentsMargins(14, 0, 10, 0);
    m_layout->setSpacing(6);
    m_layout->addWidget(m_titleLabel, 1);

    QToolButton *minimizeButton = createWindowButton(UiIcon::Minimize, tr("Minimize"));
    m_maximizeButton = createWindowButton(UiIcon::Maximize, tr("Maximize or restore"));
    QToolButton *closeButton = createWindowButton(UiIcon::Close, tr("Close"));
    closeButton->setObjectName(QStringLiteral("closeButton"));

    connect(minimizeButton, &QToolButton::clicked, this, [this] {
        hostWindow()->showMinimized();
    });
    connect(m_maximizeButton, &QToolButton::clicked, this, &TitleBar::toggleMaximized);
    connect(closeButton, &QToolButton::clicked, this, [this] {
        hostWindow()->close();
    });

    m_layout->addWidget(minimizeButton);
    m_layout->addWidget(m_maximizeButton);
    m_layout->addWidget(closeButton);
}

void TitleBar::addActionButton(QAction *action)
{
    auto *button = new QToolButton(this);
    button->setDefaultAction(action);
    button->setToolButtonStyle(Qt::ToolButtonIconOnly);
    button->setIconSize(QSize(kIconSize, kIconSize));
    button->setFixedSize(kButtonSize, kButtonSize);
    button->setFocusPolicy(Qt::NoFocus);
    m_layout->insertWidget(qMax(1, m_layout->count() - 3), button);
}

void TitleBar::setTitle(const QString &title)
{
    m_titleLabel->setText(title);
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        toggleMaximized();
        event->accept();
        return;
    }

    QWidget::mouseDoubleClickEvent(event);
}

void TitleBar::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && !m_dragOffset.isNull()) {
        hostWindow()->move(event->globalPosition().toPoint() - m_dragOffset);
        event->accept();
        return;
    }

    QWidget::mouseMoveEvent(event);
}

void TitleBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    QWidget *host = hostWindow();
    if (host->isMaximized()) {
        event->accept();
        return;
    }

    if (QWindow *window = host->windowHandle()) {
        if (window->startSystemMove()) {
            event->accept();
            return;
        }
    }

    m_dragging = true;
    m_dragOffset = event->globalPosition().toPoint() - host->frameGeometry().topLeft();
    event->accept();
}

void TitleBar::mouseReleaseEvent(QMouseEvent *event)
{
    m_dragging = false;
    m_dragOffset = {};
    QWidget::mouseReleaseEvent(event);
}

QToolButton *TitleBar::createWindowButton(UiIcon icon, const QString &tooltip)
{
    auto *button = new QToolButton(this);
    button->setIcon(makeUiIcon(icon));
    button->setIconSize(QSize(kIconSize, kIconSize));
    button->setFixedSize(kButtonSize, kButtonSize);
    button->setToolTip(tooltip);
    button->setToolButtonStyle(Qt::ToolButtonIconOnly);
    button->setFocusPolicy(Qt::NoFocus);
    return button;
}

QWidget *TitleBar::hostWindow() const
{
    return window();
}

void TitleBar::toggleMaximized()
{
    QWidget *host = hostWindow();
    host->isMaximized() ? host->showNormal() : host->showMaximized();
}
