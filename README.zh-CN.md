# Qt 图片查看器

[English README](README.md)

Qt 图片查看器是一个基于 Qt 6 的桌面图片查看器，用于浏览本地图片文件和图片目录。它重点处理快速浏览、清晰缩放、目录缩略图，以及适合 niri 等 Wayland 窗口管理器使用的只显示图片悬浮模式。

## 功能

- 支持打开单个图片文件或图片目录。
- 异步加载目录缩略图；预览侧栏默认隐藏，可通过 `Ctrl+B` 展开或收起。
- 支持本机 Qt 图片插件提供的格式，通常包括 `png`、`jpg`、`jpeg`、`bmp`、`gif`、`webp`、`tiff`、`svg`、`svgz`。
- 支持 GIF 动画播放。
- 使用自定义无边框标题栏，仅保留图标按钮，不使用菜单栏。
- 使用浅色主题。
- 支持鼠标滚轮缩放、标题栏按钮缩放、鼠标中键拖动图片和键盘切换图片。
- 支持 `Ctrl+L` 向左旋转，`Ctrl+R` 向右旋转。
- 支持 `Ctrl+0` 适应窗口，`Ctrl+1` 恢复实际尺寸。
- 支持 `Ctrl+T` 进入只显示图片的悬浮模式；悬浮窗口无边框，并在窗口管理器接受提示时保持置顶。
- 悬浮模式右键菜单支持复制、保存、旋转、透明度调整和关闭。
- 根据 `QT_IMG_VIEWER_LANG`、`LC_ALL`、`LC_MESSAGES`、`LANG` 或系统语言加载中文或英文界面。

## 编译

Arch Linux 依赖：

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

Arch Linux 打包模板位于 `packaging/aur/PKGBUILD`。它使用 CMake 安装规则，因此包内会包含可执行文件、desktop 文件、SVG 图标和许可证。

## Niri

程序使用 `qt-img-viewer` 作为 Wayland app-id。悬浮模式会隐藏标题栏、缩略图侧栏和状态栏，然后设置 Qt 的 `WindowStaysOnTopHint`。Wayland 下最终置顶行为取决于窗口管理器策略。如果 niri 没有遵循客户端置顶提示，可以在 niri 规则中匹配 app-id `qt-img-viewer`。

## 许可证

MIT License。
