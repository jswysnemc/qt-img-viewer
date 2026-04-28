#include "mainwindow.h"

#include "imageview.h"
#include "titlebar.h"
#include "uiicons.h"

#include <QAction>
#include <QApplication>
#include <QBrush>
#include <QClipboard>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QImageReader>
#include <QKeySequence>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QMovie>
#include <QPainter>
#include <QPixmap>
#include <QScreen>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QSizeGrip>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTransform>
#include <QVBoxLayout>
#include <QWidget>
#include <QWindow>
#include <QtConcurrent>

#include <algorithm>

namespace {
constexpr int kThumbnailSize = 128;
constexpr int kThumbnailIconSize = 150;

QString defaultPicturesDirectory()
{
    const QString pictures = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    return pictures.isEmpty() ? QDir::homePath() : pictures;
}

QString appStyle()
{
    return QStringLiteral(
        "QMainWindow {"
        "  background: #f5f2eb;"
        "}"
        "QWidget {"
        "  color: #263241;"
        "  selection-background-color: #2f63d8;"
        "  selection-color: #ffffff;"
        "}"
        "QSplitter {"
        "  background: #f5f2eb;"
        "}"
        "QSplitter::handle {"
        "  background: #ddd7cd;"
        "}"
        "QSplitter::handle:horizontal {"
        "  width: 2px;"
        "}"
        "QListWidget {"
        "  background: #fbfaf7;"
        "  border: 0;"
        "  border-right: 1px solid #ddd7cd;"
        "  color: #263241;"
        "  outline: 0;"
        "}"
        "QListWidget::item {"
        "  border: 1px solid transparent;"
        "  border-radius: 14px;"
        "  padding: 8px 6px;"
        "  margin: 7px;"
        "}"
        "QListWidget::item:hover {"
        "  background: #f0ebe2;"
        "  border-color: #d8d0c3;"
        "}"
        "QListWidget::item:selected {"
        "  background: #dce8ff;"
        "  border-color: #2f63d8;"
        "  color: #163a78;"
        "}"
        "QMenu {"
        "  background: #fbfaf7;"
        "  border: 1px solid #d8d0c3;"
        "  color: #263241;"
        "  padding: 6px;"
        "}"
        "QMenu::item {"
        "  border-radius: 6px;"
        "  padding: 6px 22px;"
        "}"
        "QMenu::item:selected {"
        "  background: #dce8ff;"
        "  color: #163a78;"
        "}"
        "QMenu::separator {"
        "  background: #e2ddd3;"
        "  height: 1px;"
        "  margin: 6px 4px;"
        "}"
        "QToolTip {"
        "  background: #263241;"
        "  border: 0;"
        "  color: #ffffff;"
        "  padding: 5px;"
        "}"
        "QStatusBar {"
        "  background: #fbfaf7;"
        "  border-top: 1px solid #ddd7cd;"
        "  color: #6a7280;"
        "}"
        "QStatusBar QLabel {"
        "  color: #263241;"
        "}"
        "QScrollBar:vertical {"
        "  background: #fbfaf7;"
        "  width: 10px;"
        "  margin: 0;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: #c8c0b3;"
        "  border-radius: 5px;"
        "  min-height: 28px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "  background: #afa596;"
        "}"
        "QScrollBar::add-line:vertical,"
        "QScrollBar::sub-line:vertical {"
        "  height: 0;"
        "}"
        "QScrollBar:horizontal {"
        "  background: #fbfaf7;"
        "  height: 10px;"
        "  margin: 0;"
        "}"
        "QScrollBar::handle:horizontal {"
        "  background: #c8c0b3;"
        "  border-radius: 5px;"
        "  min-width: 28px;"
        "}"
        "QScrollBar::handle:horizontal:hover {"
        "  background: #afa596;"
        "}"
        "QScrollBar::add-line:horizontal,"
        "QScrollBar::sub-line:horizontal {"
        "  width: 0;"
        "}");
}

QIcon thumbnailPlaceholderIcon()
{
    QPixmap pixmap(kThumbnailIconSize, kThumbnailIconSize);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(QColor(216, 208, 195), 1.4));
    painter.setBrush(QColor(248, 246, 241));
    painter.drawRoundedRect(QRectF(10, 10, kThumbnailIconSize - 20, kThumbnailIconSize - 20), 18, 18);

    painter.setPen(QPen(QColor(142, 151, 166), 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawRect(QRectF(48, 44, 54, 62));
    painter.drawLine(QPointF(58, 86), QPointF(71, 72));
    painter.drawLine(QPointF(71, 72), QPointF(84, 90));
    painter.drawEllipse(QRectF(80, 55, 10, 10));

    return QIcon(pixmap);
}

ImageLoadResult readImageFile(const QString &path)
{
    QImageReader reader(path);
    reader.setAutoTransform(true);

    ImageLoadResult result;
    result.path = path;
    result.image = reader.read();
    if (result.image.isNull()) {
        result.error = reader.errorString();
    }
    return result;
}

ThumbnailLoadResult readThumbnailFile(const QString &path)
{
    QImageReader reader(path);
    reader.setAutoTransform(true);
    QSize scaledSize = reader.size();
    if (scaledSize.isValid()) {
        scaledSize.scale(kThumbnailIconSize, kThumbnailIconSize, Qt::KeepAspectRatio);
        reader.setScaledSize(scaledSize);
    }

    ThumbnailLoadResult result;
    result.path = path;
    result.image = reader.read();
    return result;
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_imageView(new ImageView(this))
    , m_thumbnailList(new QListWidget(this))
    , m_splitter(new QSplitter(this))
    , m_titleBar(new TitleBar(this))
    , m_zoomLabel(new QLabel(this))
{
    setWindowFlag(Qt::FramelessWindowHint, true);
    setStyleSheet(appStyle());

    for (const QByteArray &format : QImageReader::supportedImageFormats()) {
        m_supportedSuffixes.insert(QString::fromLatin1(format).toLower());
    }

    m_thumbnailList->setViewMode(QListView::IconMode);
    m_thumbnailList->setIconSize(QSize(kThumbnailIconSize, kThumbnailIconSize));
    m_thumbnailList->setResizeMode(QListView::Adjust);
    m_thumbnailList->setMovement(QListView::Static);
    m_thumbnailList->setUniformItemSizes(true);
    m_thumbnailList->setWordWrap(true);
    m_thumbnailList->setSpacing(2);
    m_thumbnailList->setGridSize(QSize(182, 196));
    m_thumbnailList->setTextElideMode(Qt::ElideMiddle);
    m_thumbnailList->setMinimumWidth(196);
    m_thumbnailList->setMaximumWidth(390);
    m_thumbnailList->setFrameShape(QFrame::NoFrame);

    m_splitter->addWidget(m_thumbnailList);
    m_splitter->addWidget(m_imageView);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    m_thumbnailPanelSizes = {220, 960};

    buildActions();
    buildTitleBar();
    buildStatusBar();

    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_titleBar);
    layout->addWidget(m_splitter, 1);
    setCentralWidget(central);

    connect(m_thumbnailList, &QListWidget::itemActivated, this, &MainWindow::openSelectedThumbnail);
    connect(m_thumbnailList, &QListWidget::itemClicked, this, &MainWindow::openSelectedThumbnail);
    connect(m_imageView, &ImageView::contextMenuRequested, this, &MainWindow::showFloatingContextMenu);
    connect(m_imageView, &ImageView::zoomChanged, this, &MainWindow::updateZoomLabel);
    connect(m_imageView, &ImageView::zoomChanged, this, [this] {
        if (m_imageOnlyMode) {
            resizeFloatingWindowToImage();
        }
    });

    m_toggleThumbnailsAction->setChecked(true);
    m_thumbnailList->hide();
    m_splitter->setSizes({0, 1});

    resize(1180, 760);
    if (QScreen *screen = windowHandle() ? windowHandle()->screen() : QApplication::primaryScreen()) {
        move(screen->availableGeometry().center() - rect().center());
    }

    updateActions();
    updateWindowTitle();
}

void MainWindow::openPath(const QString &path)
{
    const QFileInfo fileInfo(path);
    if (fileInfo.isDir()) {
        loadDirectory(fileInfo.absoluteFilePath());
        return;
    }

    if (fileInfo.isFile()) {
        loadDirectory(fileInfo.absolutePath(), fileInfo.absoluteFilePath());
    }
}

void MainWindow::openDirectory()
{
    const QString startDir = m_currentDirectory.isEmpty() ? defaultPicturesDirectory() : m_currentDirectory;
    const QString directoryPath = QFileDialog::getExistingDirectory(this, tr("Open image directory"), startDir);
    if (!directoryPath.isEmpty()) {
        loadDirectory(directoryPath);
    }
}

void MainWindow::openFile()
{
    const QString startDir = m_currentDirectory.isEmpty() ? defaultPicturesDirectory() : m_currentDirectory;
    const QString path = QFileDialog::getOpenFileName(this, tr("Open image"), startDir, supportedFormatFilter());
    if (path.isEmpty()) {
        return;
    }

    loadDirectory(QFileInfo(path).absolutePath(), path);
}

void MainWindow::openSelectedThumbnail(QListWidgetItem *item)
{
    if (!item) {
        return;
    }

    loadImage(item->data(Qt::UserRole).toString());
}

void MainWindow::openNextImage()
{
    if (m_images.isEmpty()) {
        return;
    }

    const int currentIndex = m_images.indexOf(m_imageView->imagePath());
    const int nextIndex = currentIndex < 0 ? 0 : (currentIndex + 1) % m_images.size();
    loadImage(m_images.at(nextIndex));
}

void MainWindow::openPreviousImage()
{
    if (m_images.isEmpty()) {
        return;
    }

    const int currentIndex = m_images.indexOf(m_imageView->imagePath());
    const int previousIndex = currentIndex < 0 ? 0 : (currentIndex + m_images.size() - 1) % m_images.size();
    loadImage(m_images.at(previousIndex));
}

void MainWindow::setAlwaysOnTop(bool enabled)
{
    applyImageOnlyMode(enabled);
}

void MainWindow::showFloatingContextMenu(const QPoint &globalPos)
{
    if (!m_imageOnlyMode || !m_imageView->hasImage()) {
        return;
    }

    QMenu menu(this);
    menu.setStyleSheet(QStringLiteral(
        "QMenu {"
        "  background: #f4faf3;"
        "  color: #102218;"
        "  border: 1px solid #c6d7c5;"
        "  padding: 4px 0;"
        "}"
        "QMenu::item {"
        "  padding: 7px 20px;"
        "}"
        "QMenu::item:selected {"
        "  background: #d8ebd6;"
        "}"));

    menu.addAction(tr("Copy to clipboard"), this, &MainWindow::copyFloatingImageToClipboard);
    menu.addAction(tr("Save to file"), this, &MainWindow::saveFloatingImageAs);
    menu.addSeparator();
    menu.addAction(tr("Rotate right"), this, [this] {
        rotateImage(90);
    });
    menu.addAction(tr("Rotate left"), this, [this] {
        rotateImage(-90);
    });
    menu.addSeparator();
    menu.addAction(tr("Increase opacity"), this, [this] {
        adjustFloatingOpacity(0.1);
    });
    menu.addAction(tr("Decrease opacity"), this, [this] {
        adjustFloatingOpacity(-0.1);
    });
    menu.addSeparator();
    menu.addAction(tr("Close"), this, &MainWindow::closeFloatingImageMode);

    menu.exec(globalPos);
}

void MainWindow::showAbout()
{
    QMessageBox::about(this,
                       tr("About Qt Image Viewer"),
                       tr("A compact Qt image viewer with directory thumbnails, zoom, drag navigation, and an always-on-top toggle."));
}

void MainWindow::toggleThumbnailPanel()
{
    if (m_imageOnlyMode) {
        return;
    }

    const bool showPanel = !m_toggleThumbnailsAction->isChecked();
    if (!showPanel) {
        m_thumbnailPanelSizes = m_splitter->sizes();
    }

    m_thumbnailList->setVisible(showPanel);
    if (showPanel) {
        if (m_thumbnailPanelSizes.size() == 2 && m_thumbnailPanelSizes.first() > 0) {
            m_splitter->setSizes(m_thumbnailPanelSizes);
        } else {
            m_splitter->setSizes({220, qMax(1, width() - 220)});
        }
    } else {
        m_splitter->setSizes({0, 1});
    }
}

void MainWindow::updateActions()
{
    const bool hasImage = m_imageView->hasImage() && !(m_imageWatcher && m_imageWatcher->isRunning());
    const bool hasMultipleImages = m_images.size() > 1;
    m_nextAction->setEnabled(hasMultipleImages);
    m_previousAction->setEnabled(hasMultipleImages);
    m_zoomInAction->setEnabled(hasImage);
    m_zoomOutAction->setEnabled(hasImage);
    m_fitAction->setEnabled(hasImage);
    m_actualSizeAction->setEnabled(hasImage);
    m_rotateLeftAction->setEnabled(hasImage);
    m_rotateRightAction->setEnabled(hasImage);
    m_toggleThumbnailsAction->setEnabled(!m_imageOnlyMode);
    m_alwaysOnTopAction->setEnabled(m_imageOnlyMode || hasImage);
    m_exitImageOnlyAction->setEnabled(m_imageOnlyMode);
}

void MainWindow::updateZoomLabel(double factor)
{
    m_zoomLabel->setText(tr("%1%").arg(qRound(factor * 100.0)));
    updateActions();
}

void MainWindow::buildActions()
{
    m_openFileAction = new QAction(tr("Open"), this);
    m_openFileAction->setShortcut(QKeySequence::Open);
    m_openFileAction->setIcon(makeUiIcon(UiIcon::OpenFile));
    m_openFileAction->setToolTip(tr("Open image (%1)").arg(m_openFileAction->shortcut().toString(QKeySequence::NativeText)));
    connect(m_openFileAction, &QAction::triggered, this, &MainWindow::openFile);

    m_openDirectoryAction = new QAction(tr("Folder"), this);
    m_openDirectoryAction->setIcon(makeUiIcon(UiIcon::OpenFolder));
    m_openDirectoryAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O));
    m_openDirectoryAction->setToolTip(tr("Open directory (%1)").arg(m_openDirectoryAction->shortcut().toString(QKeySequence::NativeText)));
    connect(m_openDirectoryAction, &QAction::triggered, this, &MainWindow::openDirectory);

    m_previousAction = new QAction(tr("Previous"), this);
    m_previousAction->setIcon(makeUiIcon(UiIcon::Previous));
    m_previousAction->setShortcut(QKeySequence(Qt::Key_Left));
    m_previousAction->setToolTip(tr("Previous image (%1)").arg(m_previousAction->shortcut().toString(QKeySequence::NativeText)));
    connect(m_previousAction, &QAction::triggered, this, &MainWindow::openPreviousImage);

    m_nextAction = new QAction(tr("Next"), this);
    m_nextAction->setIcon(makeUiIcon(UiIcon::Next));
    m_nextAction->setShortcut(QKeySequence(Qt::Key_Right));
    m_nextAction->setToolTip(tr("Next image (%1)").arg(m_nextAction->shortcut().toString(QKeySequence::NativeText)));
    connect(m_nextAction, &QAction::triggered, this, &MainWindow::openNextImage);

    m_previousAction->setShortcutContext(Qt::ApplicationShortcut);
    m_nextAction->setShortcutContext(Qt::ApplicationShortcut);

    m_zoomInAction = new QAction(tr("Zoom +"), this);
    m_zoomInAction->setIcon(makeUiIcon(UiIcon::ZoomIn));
    m_zoomInAction->setShortcut(QKeySequence::ZoomIn);
    m_zoomInAction->setToolTip(tr("Zoom in (%1)").arg(m_zoomInAction->shortcut().toString(QKeySequence::NativeText)));
    connect(m_zoomInAction, &QAction::triggered, m_imageView, &ImageView::zoomIn);

    m_zoomOutAction = new QAction(tr("Zoom -"), this);
    m_zoomOutAction->setIcon(makeUiIcon(UiIcon::ZoomOut));
    m_zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    m_zoomOutAction->setToolTip(tr("Zoom out (%1)").arg(m_zoomOutAction->shortcut().toString(QKeySequence::NativeText)));
    connect(m_zoomOutAction, &QAction::triggered, m_imageView, &ImageView::zoomOut);

    m_fitAction = new QAction(tr("Fit"), this);
    m_fitAction->setIcon(makeUiIcon(UiIcon::Fit));
    m_fitAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_0));
    m_fitAction->setToolTip(tr("Fit to window (%1)").arg(m_fitAction->shortcut().toString(QKeySequence::NativeText)));
    connect(m_fitAction, &QAction::triggered, m_imageView, &ImageView::fitToWindow);

    m_actualSizeAction = new QAction(tr("1:1"), this);
    m_actualSizeAction->setIcon(makeUiIcon(UiIcon::ActualSize));
    m_actualSizeAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_1));
    m_actualSizeAction->setToolTip(tr("Actual size (%1)").arg(m_actualSizeAction->shortcut().toString(QKeySequence::NativeText)));
    connect(m_actualSizeAction, &QAction::triggered, m_imageView, &ImageView::resetZoom);

    m_rotateLeftAction = new QAction(tr("Rotate left"), this);
    m_rotateLeftAction->setIcon(makeUiIcon(UiIcon::RotateLeft));
    m_rotateLeftAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L));
    m_rotateLeftAction->setToolTip(tr("Rotate left (%1)").arg(m_rotateLeftAction->shortcut().toString(QKeySequence::NativeText)));
    connect(m_rotateLeftAction, &QAction::triggered, this, [this] {
        rotateImage(-90);
    });

    m_rotateRightAction = new QAction(tr("Rotate right"), this);
    m_rotateRightAction->setIcon(makeUiIcon(UiIcon::RotateRight));
    m_rotateRightAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    m_rotateRightAction->setToolTip(tr("Rotate right (%1)").arg(m_rotateRightAction->shortcut().toString(QKeySequence::NativeText)));
    connect(m_rotateRightAction, &QAction::triggered, this, [this] {
        rotateImage(90);
    });

    m_toggleThumbnailsAction = new QAction(tr("Toggle preview"), this);
    m_toggleThumbnailsAction->setIcon(makeUiIcon(UiIcon::Sidebar));
    m_toggleThumbnailsAction->setCheckable(true);
    m_toggleThumbnailsAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_B));
    m_toggleThumbnailsAction->setToolTip(tr("Show or collapse directory preview (%1)").arg(m_toggleThumbnailsAction->shortcut().toString(QKeySequence::NativeText)));
    connect(m_toggleThumbnailsAction, &QAction::triggered, this, &MainWindow::toggleThumbnailPanel);

    m_alwaysOnTopAction = new QAction(tr("Float image"), this);
    m_alwaysOnTopAction->setIcon(makeUiIcon(UiIcon::Pin));
    m_alwaysOnTopAction->setCheckable(true);
    m_alwaysOnTopAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_T));
    m_alwaysOnTopAction->setToolTip(tr("Show only the image and keep it on top (%1)").arg(m_alwaysOnTopAction->shortcut().toString(QKeySequence::NativeText)));
    connect(m_alwaysOnTopAction, &QAction::toggled, this, [this](bool checked) {
        setAlwaysOnTop(checked);
    });

    m_exitImageOnlyAction = new QAction(tr("Exit image-only mode"), this);
    m_exitImageOnlyAction->setShortcut(QKeySequence(Qt::Key_Escape));
    connect(m_exitImageOnlyAction, &QAction::triggered, this, [this] {
        if (m_imageOnlyMode) {
            m_alwaysOnTopAction->setChecked(false);
        }
    });

    m_aboutAction = new QAction(tr("About"), this);
    m_aboutAction->setIcon(makeUiIcon(UiIcon::Info));
    m_aboutAction->setToolTip(tr("About this application"));
    connect(m_aboutAction, &QAction::triggered, this, &MainWindow::showAbout);

    const QList<QAction *> actions = {
        m_openFileAction,
        m_openDirectoryAction,
        m_previousAction,
        m_nextAction,
        m_zoomInAction,
        m_zoomOutAction,
        m_fitAction,
        m_actualSizeAction,
        m_rotateLeftAction,
        m_rotateRightAction,
        m_toggleThumbnailsAction,
        m_alwaysOnTopAction,
        m_exitImageOnlyAction,
        m_aboutAction,
    };

    for (QAction *action : actions) {
        addAction(action);
    }
}

void MainWindow::buildStatusBar()
{
    m_zoomLabel->setMinimumWidth(72);
    m_zoomLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    statusBar()->addPermanentWidget(m_zoomLabel);
    statusBar()->addPermanentWidget(new QSizeGrip(this), 0);
    updateZoomLabel(1.0);
}

void MainWindow::buildTitleBar()
{
    m_titleBar->addActionButton(m_openFileAction);
    m_titleBar->addActionButton(m_openDirectoryAction);
    m_titleBar->addActionButton(m_previousAction);
    m_titleBar->addActionButton(m_nextAction);
    m_titleBar->addActionButton(m_zoomOutAction);
    m_titleBar->addActionButton(m_zoomInAction);
    m_titleBar->addActionButton(m_fitAction);
    m_titleBar->addActionButton(m_actualSizeAction);
    m_titleBar->addActionButton(m_rotateLeftAction);
    m_titleBar->addActionButton(m_rotateRightAction);
    m_titleBar->addActionButton(m_toggleThumbnailsAction);
    m_titleBar->addActionButton(m_alwaysOnTopAction);
    m_titleBar->addActionButton(m_aboutAction);
}

void MainWindow::applyImageOnlyMode(bool enabled)
{
    if (enabled && !m_imageView->hasImage()) {
        QSignalBlocker blocker(m_alwaysOnTopAction);
        m_alwaysOnTopAction->setChecked(false);
        updateActions();
        return;
    }

    if (m_imageOnlyMode == enabled) {
        const Qt::WindowFlags flags = windowFlags();
        setWindowFlags(enabled ? (flags | Qt::WindowStaysOnTopHint) : (flags & ~Qt::WindowStaysOnTopHint));
        show();
        return;
    }

    m_imageOnlyMode = enabled;

    if (enabled) {
        m_browserGeometry = geometry();
        m_browserSplitterSizes = m_splitter->sizes();
        m_browserMinimumSize = minimumSize();
        m_browserMaximumSize = maximumSize();
    }

    m_titleBar->setVisible(!enabled);
    m_thumbnailList->setVisible(!enabled);
    statusBar()->setVisible(!enabled);
    m_imageView->setHorizontalScrollBarPolicy(enabled ? Qt::ScrollBarAlwaysOff : Qt::ScrollBarAsNeeded);
    m_imageView->setVerticalScrollBarPolicy(enabled ? Qt::ScrollBarAlwaysOff : Qt::ScrollBarAsNeeded);
    m_imageView->setResizeFitEnabled(!enabled);
    m_imageView->setWindowDragEnabled(enabled);
    if (enabled) {
        m_imageView->setImageOpacity(1.0);
    }

    Qt::WindowFlags flags = windowFlags();
    flags |= Qt::FramelessWindowHint;
    if (enabled) {
        flags |= Qt::WindowStaysOnTopHint;
    } else {
        flags &= ~Qt::WindowStaysOnTopHint;
    }
    setWindowFlags(flags);

    if (!enabled && m_browserGeometry.isValid()) {
        setMinimumSize(m_browserMinimumSize);
        setMaximumSize(m_browserMaximumSize);
        setGeometry(m_browserGeometry);
        if (!m_browserSplitterSizes.isEmpty()) {
            m_splitter->setSizes(m_browserSplitterSizes);
        }
    }

    show();
    raise();
    activateWindow();
    if (enabled) {
        m_imageView->resetZoom();
        resizeFloatingWindowToImage();
    } else {
        m_imageView->fitToWindow();
    }
    updateWindowTitle();
    updateActions();
}

bool MainWindow::canReadImage(const QString &path) const
{
    const QFileInfo fileInfo(path);
    return fileInfo.isFile() && m_supportedSuffixes.contains(fileInfo.suffix().toLower());
}

QStringList MainWindow::collectImages(const QString &directoryPath) const
{
    QStringList images;
    const QDir directory(directoryPath);
    const QFileInfoList entries = directory.entryInfoList(QDir::Files | QDir::Readable, QDir::Name | QDir::IgnoreCase);
    for (const QFileInfo &entry : entries) {
        if (canReadImage(entry.absoluteFilePath())) {
            images.append(entry.absoluteFilePath());
        }
    }
    return images;
}

void MainWindow::adjustFloatingOpacity(double delta)
{
    if (!m_imageOnlyMode) {
        return;
    }

    m_imageView->setImageOpacity(m_imageView->imageOpacity() + delta);
}

void MainWindow::closeFloatingImageMode()
{
    if (m_imageOnlyMode) {
        m_alwaysOnTopAction->setChecked(false);
    }
}

void MainWindow::copyFloatingImageToClipboard()
{
    const QPixmap pixmap = m_imageView->pixmap();
    if (!pixmap.isNull()) {
        QApplication::clipboard()->setPixmap(pixmap);
    }
}

void MainWindow::loadDirectory(const QString &directoryPath, const QString &preferredPath)
{
    ++m_thumbnailGeneration;
    if (m_thumbnailWatcher) {
        QFutureWatcher<ThumbnailLoadResult> *oldWatcher = m_thumbnailWatcher;
        m_thumbnailWatcher = nullptr;
        disconnect(oldWatcher, nullptr, this, nullptr);
        oldWatcher->cancel();
        connect(oldWatcher, &QFutureWatcher<ThumbnailLoadResult>::finished, oldWatcher, &QObject::deleteLater);
    }

    const QStringList images = collectImages(directoryPath);
    if (images.isEmpty()) {
        QMessageBox::information(this, tr("No images"), tr("This directory has no supported images."));
        return;
    }

    m_currentDirectory = directoryPath;
    m_images = images;
    m_itemsByPath.clear();
    m_thumbnailList->clear();

    const QIcon placeholder = thumbnailPlaceholderIcon();
    for (const QString &path : m_images) {
        auto *item = new QListWidgetItem(placeholder, QFileInfo(path).fileName(), m_thumbnailList);
        item->setData(Qt::UserRole, path);
        item->setToolTip(path);
        item->setTextAlignment(Qt::AlignCenter);
        m_itemsByPath.insert(path, item);
    }

    const QString firstPath = preferredPath.isEmpty() ? m_images.first() : preferredPath;
    loadImage(firstPath);
    startThumbnailLoading(m_thumbnailGeneration);
}

bool MainWindow::loadImage(const QString &path)
{
    ++m_imageLoadGeneration;
    const int generation = m_imageLoadGeneration;

    stopMovie();

    if (m_imageWatcher) {
        QFutureWatcher<ImageLoadResult> *oldWatcher = m_imageWatcher;
        m_imageWatcher = nullptr;
        disconnect(oldWatcher, nullptr, this, nullptr);
        oldWatcher->cancel();
        connect(oldWatcher, &QFutureWatcher<ImageLoadResult>::finished, oldWatcher, &QObject::deleteLater);
    }

    selectThumbnail(path);
    statusBar()->showMessage(tr("Loading %1").arg(path));
    updateActions();

    if (QFileInfo(path).suffix().compare(QStringLiteral("gif"), Qt::CaseInsensitive) == 0) {
        return loadAnimatedGif(path, generation);
    }

    auto *watcher = new QFutureWatcher<ImageLoadResult>(this);
    m_imageWatcher = watcher;
    connect(watcher, &QFutureWatcher<ImageLoadResult>::finished, this, [this, watcher, generation] {
        watcher->deleteLater();
        if (watcher != m_imageWatcher) {
            return;
        }
        m_imageWatcher = nullptr;
        if (generation != m_imageLoadGeneration) {
            return;
        }

        const ImageLoadResult result = watcher->result();
        if (result.image.isNull()) {
            QMessageBox::warning(this, tr("Cannot open image"), tr("Failed to read image:\n%1\n\n%2").arg(result.path, result.error));
            updateActions();
            return;
        }

        m_imageView->setImage(QPixmap::fromImage(result.image), result.path);
        const int imageIndex = m_images.indexOf(result.path);
        const QString position = imageIndex >= 0 ? tr("  %1 / %2").arg(imageIndex + 1).arg(m_images.size()) : QString();
        statusBar()->showMessage(tr("%1  %2 x %3%4").arg(result.path).arg(result.image.width()).arg(result.image.height()).arg(position));
        selectThumbnail(result.path);
        resizeFloatingWindowToImage();
        updateWindowTitle();
        updateActions();
    });

    watcher->setFuture(QtConcurrent::run(readImageFile, path));
    return true;
}

bool MainWindow::loadAnimatedGif(const QString &path, int generation)
{
    auto *movie = new QMovie(path, QByteArray(), this);
    movie->setCacheMode(QMovie::CacheAll);
    if (!movie->isValid()) {
        const QString error = movie->lastErrorString();
        movie->deleteLater();
        QMessageBox::warning(this, tr("Cannot open image"), tr("Failed to read image:\n%1\n\n%2").arg(path, error));
        updateActions();
        return false;
    }

    m_movie = movie;
    connect(movie, &QMovie::frameChanged, this, [this, movie, generation] {
        if (movie != m_movie || generation != m_imageLoadGeneration) {
            return;
        }

        const QPixmap frame = movie->currentPixmap();
        if (frame.isNull()) {
            return;
        }

        m_imageView->setImageFrame(frame);
        if (m_imageOnlyMode) {
            resizeFloatingWindowToImage();
        }
    });

    movie->jumpToFrame(0);
    const QPixmap firstFrame = movie->currentPixmap();
    if (firstFrame.isNull()) {
        const QString error = movie->lastErrorString();
        stopMovie();
        QMessageBox::warning(this, tr("Cannot open image"), tr("Failed to read image:\n%1\n\n%2").arg(path, error));
        updateActions();
        return false;
    }

    m_imageView->setImage(firstFrame, path);
    const int imageIndex = m_images.indexOf(path);
    const QString position = imageIndex >= 0 ? tr("  %1 / %2").arg(imageIndex + 1).arg(m_images.size()) : QString();
    statusBar()->showMessage(tr("%1  %2 x %3  GIF%4").arg(path).arg(firstFrame.width()).arg(firstFrame.height()).arg(position));
    selectThumbnail(path);
    resizeFloatingWindowToImage();
    updateWindowTitle();
    updateActions();

    movie->start();
    return true;
}

void MainWindow::rotateImage(int degrees)
{
    if (!m_imageView->hasImage()) {
        return;
    }

    QTransform transform;
    transform.rotate(degrees);
    const QPixmap rotated = m_imageView->pixmap().transformed(transform, Qt::SmoothTransformation);
    if (rotated.isNull()) {
        return;
    }

    stopMovie();
    m_imageView->setImage(rotated, m_imageView->imagePath());
    if (m_imageOnlyMode) {
        m_imageView->setResizeFitEnabled(false);
        m_imageView->resetZoom();
        resizeFloatingWindowToImage();
    }
}

void MainWindow::saveFloatingImageAs()
{
    const QPixmap pixmap = m_imageView->pixmap();
    if (pixmap.isNull()) {
        return;
    }

    const QFileInfo currentInfo(m_imageView->imagePath());
    const QString baseName = currentInfo.completeBaseName().isEmpty() ? QStringLiteral("image") : currentInfo.completeBaseName();
    const QString startDir = m_currentDirectory.isEmpty() ? defaultPicturesDirectory() : m_currentDirectory;
    const QString defaultPath = QDir(startDir).filePath(baseName + QStringLiteral(".png"));
    const QString path = QFileDialog::getSaveFileName(this, tr("Save image"), defaultPath, tr("PNG image (*.png);;JPEG image (*.jpg *.jpeg);;All files (*)"));
    if (path.isEmpty()) {
        return;
    }

    if (!pixmap.save(path)) {
        QMessageBox::warning(this, tr("Cannot save image"), tr("Failed to save image:\n%1").arg(path));
    }
}

void MainWindow::resizeFloatingWindowToImage()
{
    if (!m_imageOnlyMode || !m_imageView->hasImage()) {
        return;
    }

    const QSize imageSize = m_imageView->imageSize();
    if (!imageSize.isValid()) {
        return;
    }
    QSize targetViewportSize(qMax(1, qRound(imageSize.width() * m_imageView->zoomFactor())),
                             qMax(1, qRound(imageSize.height() * m_imageView->zoomFactor())));

    QScreen *screen = windowHandle() ? windowHandle()->screen() : QApplication::primaryScreen();
    if (!screen) {
        resize(targetViewportSize);
        return;
    }

    const QRect available = screen->availableGeometry();
    QSize maxViewportSize = available.size();
    maxViewportSize.rwidth() = qMax(1, int(maxViewportSize.width() * 0.98));
    maxViewportSize.rheight() = qMax(1, int(maxViewportSize.height() * 0.98));

    targetViewportSize.setWidth(qMin(targetViewportSize.width(), maxViewportSize.width()));
    targetViewportSize.setHeight(qMin(targetViewportSize.height(), maxViewportSize.height()));

    setMinimumSize(1, 1);
    setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);

    for (int pass = 0; pass < 2; ++pass) {
        const QSize viewportSize = m_imageView->viewport() ? m_imageView->viewport()->size() : m_imageView->size();
        const QSize frameDelta(qMax(0, width() - viewportSize.width()),
                               qMax(0, height() - viewportSize.height()));
        QSize targetWindowSize = targetViewportSize + frameDelta;
        targetWindowSize.setWidth(qMin(targetWindowSize.width(), available.width()));
        targetWindowSize.setHeight(qMin(targetWindowSize.height(), available.height()));

        resize(targetWindowSize);
        move(available.center() - rect().center());
    }
}

void MainWindow::selectThumbnail(const QString &path)
{
    QListWidgetItem *item = m_itemsByPath.value(path, nullptr);
    if (!item) {
        return;
    }

    m_thumbnailList->setCurrentItem(item);
    m_thumbnailList->scrollToItem(item, QAbstractItemView::PositionAtCenter);
}

void MainWindow::startThumbnailLoading(int generation)
{
    auto *watcher = new QFutureWatcher<ThumbnailLoadResult>(this);
    m_thumbnailWatcher = watcher;

    connect(watcher, &QFutureWatcher<ThumbnailLoadResult>::resultReadyAt, this, [this, watcher, generation](int index) {
        if (watcher != m_thumbnailWatcher || generation != m_thumbnailGeneration) {
            return;
        }

        const ThumbnailLoadResult result = watcher->resultAt(index);
        if (result.image.isNull()) {
            return;
        }

        QListWidgetItem *item = m_itemsByPath.value(result.path, nullptr);
        if (!item) {
            return;
        }

        item->setIcon(QIcon(QPixmap::fromImage(result.image)));
    });

    connect(watcher, &QFutureWatcher<ThumbnailLoadResult>::finished, this, [this, watcher, generation] {
        watcher->deleteLater();
        if (watcher == m_thumbnailWatcher) {
            m_thumbnailWatcher = nullptr;
        }
        if (generation == m_thumbnailGeneration) {
            statusBar()->showMessage(tr("Loaded %1 images").arg(m_images.size()), 3000);
        }
    });

    statusBar()->showMessage(tr("Loading thumbnails..."));
    watcher->setFuture(QtConcurrent::mapped(m_images, readThumbnailFile));
}

void MainWindow::stopMovie()
{
    if (!m_movie) {
        return;
    }

    QMovie *movie = m_movie;
    m_movie = nullptr;
    disconnect(movie, nullptr, this, nullptr);
    movie->stop();
    movie->deleteLater();
}

QString MainWindow::supportedFormatFilter() const
{
    QStringList patterns;
    for (const QString &suffix : m_supportedSuffixes) {
        patterns.append(QStringLiteral("*.%1").arg(suffix));
    }
    patterns.sort(Qt::CaseInsensitive);
    return tr("Images (%1);;All files (*)").arg(patterns.join(' '));
}

void MainWindow::updateWindowTitle()
{
    QString title = tr("Qt Image Viewer");
    if (m_imageView->hasImage()) {
        title = QFileInfo(m_imageView->imagePath()).fileName() + QStringLiteral(" - ") + title;
    }
    if (m_alwaysOnTopAction && m_alwaysOnTopAction->isChecked()) {
        title = tr("Floating: %1").arg(title);
    }
    setWindowTitle(title);
    if (m_titleBar) {
        m_titleBar->setTitle(title);
    }
}
