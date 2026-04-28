# Qt Image Viewer

Qt Image Viewer is a Qt 6 image viewer for local image browsing. It supports directory previews, asynchronous thumbnail loading, zooming, panning, rotation, animated GIF playback, and an image-only floating mode for Wayland compositors such as niri.

## Features

- Opens a single image file or a directory.
- Shows directory images as thumbnails, with the preview panel hidden by default and toggleable with `Ctrl+B`.
- Supports the formats exposed by the local Qt image plugins, typically including `png`, `jpg`, `jpeg`, `bmp`, `gif`, `webp`, `tiff`, `svg`, and `svgz`.
- Plays animated GIF files through `QMovie`.
- Uses a custom frameless title bar with icon-only actions and no menu bar.
- Uses a light theme with clear selected and checked states.
- Zooms under the cursor with the mouse wheel or toolbar actions.
- Pans the image with middle mouse drag in both normal mode and floating mode.
- Moves the floating window with left mouse drag in floating mode.
- Switches images with left and right arrow keys.
- Rotates left with `Ctrl+L` and rotates right with `Ctrl+R`.
- Fits to the viewport with `Ctrl+0` and resets to actual size with `Ctrl+1`.
- Enters image-only floating mode with `Ctrl+T`; this hides chrome, removes the frame, and keeps the image window on top.
- Exits image-only floating mode with `Esc`.
- Shows a floating-mode context menu for copy, save, rotation, opacity changes, and close.
- Loads translations from Qt Linguist resources according to `QT_IMG_VIEWER_LANG`, `LC_ALL`, `LC_MESSAGES`, `LANG`, or the system locale.

## Build

Install Qt 6 development packages, CMake, and a C++17 compiler. On Arch Linux:

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

The Arch Linux packaging template is in `packaging/aur/PKGBUILD`. The AUR workflow updates `pkgver` from the release tag, refreshes checksums, generates `.SRCINFO`, and pushes to `ssh://aur@aur.archlinux.org/qt-img-viewer.git`.

Set the AUR SSH private key for this GitHub repository:

```bash
gh secret set AUR_SSH_PRIVATE_KEY \
    --repo jswysnemc/qt-img-viewer \
    < ~/.ssh/id_rsa
```

## GitHub Workflows

- `.github/workflows/build.yml` builds the project on push, pull request, and version tags.
- `.github/workflows/aur-publish.yml` publishes the AUR package after a GitHub release is published, or through manual dispatch with a tag.

## Niri

The application uses `qt-img-viewer` as its Wayland app-id. Floating mode hides the title bar, thumbnail panel, and status bar, then applies Qt's `WindowStaysOnTopHint`. On Wayland, final stacking behavior depends on compositor policy. If niri does not honor the client-side always-on-top hint in your setup, match niri rules against app-id `qt-img-viewer`.

## License

MIT License.

---

# Qt 图片查看器

Qt 图片查看器是一个基于 Qt 6 的本地图片查看器。它支持目录预览、异步缩略图加载、缩放、拖动、旋转、GIF 动画播放，以及适合 niri 等 Wayland 窗口管理器使用的无边框置顶悬浮模式。

## 功能

- 支持打开单个图片文件或图片目录。
- 以缩略图展示同目录图片，预览侧栏默认关闭，可通过 `Ctrl+B` 展开或收起。
- 支持本机 Qt 图片插件提供的格式，通常包括 `png`、`jpg`、`jpeg`、`bmp`、`gif`、`webp`、`tiff`、`svg`、`svgz`。
- 使用 `QMovie` 播放 GIF 动画。
- 使用自定义无边框标题栏，仅保留图标按钮，不使用菜单栏。
- 使用浅色主题，并保证选中态和启用态清晰可见。
- 支持鼠标滚轮以光标位置为中心缩放，也支持标题栏按钮缩放。
- 普通模式和悬浮模式都支持鼠标中键拖动图片。
- 悬浮模式下支持鼠标左键拖动窗口。
- 支持左右方向键切换同目录图片。
- `Ctrl+L` 向左旋转，`Ctrl+R` 向右旋转。
- `Ctrl+0` 适应窗口，`Ctrl+1` 恢复实际尺寸。
- `Ctrl+T` 进入只显示图片的悬浮模式，窗口无边框并保持置顶。
- `Esc` 退出悬浮模式。
- 悬浮模式右键菜单支持复制、保存、旋转、透明度调整和关闭。
- 根据 `QT_IMG_VIEWER_LANG`、`LC_ALL`、`LC_MESSAGES`、`LANG` 或系统语言加载 Qt Linguist 翻译资源。

## 编译

安装 Qt 6 开发包、CMake 和支持 C++17 的编译器。Arch Linux 示例：

```bash
sudo pacman -S --needed base-devel cmake ninja qt6-base qt6-tools qt6-imageformats qt6-svg
```

编译并运行：

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/qt-img-viewer
```

直接打开文件或目录：

```bash
./build/qt-img-viewer /path/to/image-or-directory
```

指定界面语言：

```bash
QT_IMG_VIEWER_LANG=zh_CN ./build/qt-img-viewer
QT_IMG_VIEWER_LANG=en_US ./build/qt-img-viewer
```

安装到本地：

```bash
cmake --install build --prefix "$HOME/.local"
```

## 打包

Arch Linux AUR 打包模板位于 `packaging/aur/PKGBUILD`。AUR 工作流会根据发布标签更新 `pkgver`，刷新校验和，生成 `.SRCINFO`，并推送到 `ssh://aur@aur.archlinux.org/qt-img-viewer.git`。

给这个 GitHub 仓库设置 AUR SSH 私钥：

```bash
gh secret set AUR_SSH_PRIVATE_KEY \
    --repo jswysnemc/qt-img-viewer \
    < ~/.ssh/id_rsa
```

## GitHub 工作流

- `.github/workflows/build.yml` 会在推送、拉取请求和版本标签上编译项目。
- `.github/workflows/aur-publish.yml` 会在 GitHub Release 发布后推送 AUR 包，也支持手动传入标签触发。

## Niri

程序使用 `qt-img-viewer` 作为 Wayland app-id。悬浮模式会隐藏标题栏、缩略图侧栏和状态栏，然后设置 Qt 的 `WindowStaysOnTopHint`。Wayland 下最终置顶行为取决于窗口管理器策略。如果 niri 没有遵循客户端置顶提示，可以在 niri 规则中匹配 app-id `qt-img-viewer`。

## 许可证

MIT License。
