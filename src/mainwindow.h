#pragma once

#include <QHash>
#include <QFutureWatcher>
#include <QImage>
#include <QMainWindow>
#include <QRect>
#include <QSet>

class QAction;
class ImageView;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QMovie;
class QSplitter;
class TitleBar;

struct ImageLoadResult
{
    QString path;
    QImage image;
    QString error;
};

struct ThumbnailLoadResult
{
    QString path;
    QImage image;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    void openPath(const QString &path);

private slots:
    void openDirectory();
    void openFile();
    void openSelectedThumbnail(QListWidgetItem *item);
    void openNextImage();
    void openPreviousImage();
    void setAlwaysOnTop(bool enabled);
    void showFloatingContextMenu(const QPoint &globalPos);
    void showAbout();
    void toggleThumbnailPanel();
    void updateActions();
    void updateZoomLabel(double factor);

private:
    void buildActions();
    void buildStatusBar();
    void buildTitleBar();
    void applyImageOnlyMode(bool enabled);
    bool canReadImage(const QString &path) const;
    QStringList collectImages(const QString &directoryPath) const;
    void adjustFloatingOpacity(double delta);
    void closeFloatingImageMode();
    void copyFloatingImageToClipboard();
    bool loadAnimatedGif(const QString &path, int generation);
    void loadDirectory(const QString &directoryPath, const QString &preferredPath = {});
    bool loadImage(const QString &path);
    void resizeFloatingWindowToImage();
    void rotateImage(int degrees);
    void saveFloatingImageAs();
    void selectThumbnail(const QString &path);
    void startThumbnailLoading(int generation);
    void stopMovie();
    QString supportedFormatFilter() const;
    void updateWindowTitle();

    ImageView *m_imageView = nullptr;
    QListWidget *m_thumbnailList = nullptr;
    QSplitter *m_splitter = nullptr;
    TitleBar *m_titleBar = nullptr;
    QLabel *m_zoomLabel = nullptr;

    QAction *m_openFileAction = nullptr;
    QAction *m_openDirectoryAction = nullptr;
    QAction *m_nextAction = nullptr;
    QAction *m_previousAction = nullptr;
    QAction *m_zoomInAction = nullptr;
    QAction *m_zoomOutAction = nullptr;
    QAction *m_fitAction = nullptr;
    QAction *m_actualSizeAction = nullptr;
    QAction *m_rotateLeftAction = nullptr;
    QAction *m_rotateRightAction = nullptr;
    QAction *m_toggleThumbnailsAction = nullptr;
    QAction *m_alwaysOnTopAction = nullptr;
    QAction *m_aboutAction = nullptr;
    QAction *m_exitImageOnlyAction = nullptr;

    QString m_currentDirectory;
    QStringList m_images;
    QSet<QString> m_supportedSuffixes;
    QHash<QString, QListWidgetItem *> m_itemsByPath;
    QFutureWatcher<ImageLoadResult> *m_imageWatcher = nullptr;
    QFutureWatcher<ThumbnailLoadResult> *m_thumbnailWatcher = nullptr;
    QMovie *m_movie = nullptr;
    QRect m_browserGeometry;
    QList<int> m_browserSplitterSizes;
    QSize m_browserMinimumSize;
    QSize m_browserMaximumSize;
    QList<int> m_thumbnailPanelSizes;
    int m_imageLoadGeneration = 0;
    int m_thumbnailGeneration = 0;
    bool m_imageOnlyMode = false;
};
