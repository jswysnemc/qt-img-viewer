cmake_minimum_required(VERSION 3.21)

project(QtImageViewer VERSION 0.1.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)

find_package(Qt6 REQUIRED COMPONENTS Concurrent LinguistTools Widgets)

qt_standard_project_setup(I18N_TRANSLATED_LANGUAGES zh_CN)

qt_add_executable(qt-img-viewer
    src/imageview.cpp
    src/imageview.h
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/titlebar.cpp
    src/titlebar.h
    src/uiicons.cpp
    src/uiicons.h
)

target_link_libraries(qt-img-viewer PRIVATE Qt6::Concurrent Qt6::Widgets)

qt_add_translations(qt-img-viewer
    TS_FILES translations/qt-img-viewer_zh_CN.ts
    RESOURCE_PREFIX "/i18n"
)

set_target_properties(qt-img-viewer PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

install(TARGETS qt-img-viewer RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES data/qt-img-viewer.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES data/qt-img-viewer.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
