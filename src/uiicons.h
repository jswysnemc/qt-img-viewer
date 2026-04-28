#pragma once

#include <QIcon>

enum class UiIcon
{
    OpenFile,
    OpenFolder,
    Previous,
    Next,
    ZoomOut,
    ZoomIn,
    Fit,
    ActualSize,
    RotateLeft,
    RotateRight,
    Sidebar,
    Pin,
    Info,
    Minimize,
    Maximize,
    Close
};

QIcon makeUiIcon(UiIcon icon);
