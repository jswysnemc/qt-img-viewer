# Qt Image Viewer

[中文说明](README.zh-CN.md)

Qt Image Viewer is a Qt 6 desktop image viewer for local files and image directories. It focuses on fast browsing, clear scaling, directory thumbnails, and an image-only floating mode for Wayland compositors such as niri.

## Features

- Opens a single image file or an image directory.
- Shows directory thumbnails asynchronously; the preview panel is hidden by default and can be toggled with `Ctrl+B`.
- Supports image formats provided by the local Qt image plugins, typically including `png`, `jpg`, `jpeg`, `bmp`, `gif`, `webp`, `tiff`, `svg`, and `svgz`.
- Plays animated GIF files.
- Uses a custom frameless title bar with icon-only actions and no menu bar.
- Uses a light theme.
- Supports mouse wheel zoom, toolbar zoom, middle-button panning, and keyboard navigation.
- Supports left and right rotation with `Ctrl+L` and `Ctrl+R`.
- Supports fit-to-window with `Ctrl+0` and actual size with `Ctrl+1`.
- Supports image-only floating mode with `Ctrl+T`; the floating window is frameless and stays on top when the compositor honors the hint.
- Provides a floating-mode context menu for copy, save, rotation, opacity changes, and close.
- Loads Chinese or English UI text according to `QT_IMG_VIEWER_LANG`, `LC_ALL`, `LC_MESSAGES`, `LANG`, or the system locale.

## Build

Arch Linux dependencies:

```bash
sudo pacman -S --needed base-devel cmake ninja qt6-base qt6-tools qt6-imageformats qt6-svg
```

Build and run:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/qt-img-viewer
```

Open a file or directory directly:

```bash
./build/qt-img-viewer /path/to/image-or-directory
```

Open an image directly in floating mode:

```bash
./build/qt-img-viewer --float /path/to/image
```

Select a UI language:

```bash
QT_IMG_VIEWER_LANG=zh_CN ./build/qt-img-viewer
QT_IMG_VIEWER_LANG=en_US ./build/qt-img-viewer
```

Install locally:

```bash
cmake --install build --prefix "$HOME/.local"
```

## Packaging

The Arch Linux packaging template is in `packaging/aur/PKGBUILD`. It uses the CMake install rules, so the executable, desktop entry, SVG icon, and license are included in the package.

## Niri

The application uses `qt-img-viewer` as its Wayland app-id. Floating mode hides the title bar, thumbnail panel, and status bar, then applies Qt's `WindowStaysOnTopHint`. On Wayland, final stacking behavior depends on compositor policy. If niri does not honor the client-side always-on-top hint, match niri rules against app-id `qt-img-viewer`.

## License

MIT License.
