cmake_minimum_required(VERSION 3.16)
project(gxdm-greeter-neo LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 COMPONENTS Core Gui Widgets DBus Network Xml Svg REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(DTK2WIDGET REQUIRED IMPORTED_TARGET dtk2widget)

file(GLOB NEO_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/view/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/global_util/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/global_util/dbus/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/widgets/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/session_widgets/*.cpp"
)

set(NEO_VENDOR
    vendor/src/backend/users/users.cc
    vendor/src/backend/sessions/sessions.cc
    vendor/src/utils/logger/logger.cc
)

set(NEO_RESOURCES
    image.qrc
    logintheme.qrc
    widgets/widgetsimages.qrc
    widgets/widgetstheme.qrc
)

add_executable(gxdm-greeter-neo ${NEO_SOURCES} ${NEO_VENDOR} ${NEO_RESOURCES})

target_include_directories(gxdm-greeter-neo PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/global_util"
    "${CMAKE_CURRENT_SOURCE_DIR}/global_util/dbus"
    "${CMAKE_CURRENT_SOURCE_DIR}/widgets"
    "${CMAKE_CURRENT_SOURCE_DIR}/session_widgets"
    "${CMAKE_CURRENT_SOURCE_DIR}/view"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor"
)

target_link_libraries(gxdm-greeter-neo PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::DBus Qt6::Network Qt6::Xml Qt6::Svg
    PkgConfig::DTK2WIDGET
    X11 Xi Xtst Xcursor Xfixes Xrandr Xext
)

install(TARGETS gxdm-greeter-neo DESTINATION "${CMAKE_INSTALL_BINDIR}")
