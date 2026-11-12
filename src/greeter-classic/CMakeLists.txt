cmake_minimum_required(VERSION 3.16)
project(gxdm-greeter-classic LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# The legacy daemon enables these Qt porting guards for all of its children.
# The DTK greeter still uses implicit QString conversions and Qt's foreach.
remove_definitions(
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_FROM_BYTEARRAY
    -DQT_NO_FOREACH
)

find_package(Qt6 COMPONENTS Core Gui Widgets DBus Network Xml Svg REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(DTK2WIDGET REQUIRED IMPORTED_TARGET dtk2widget)
pkg_check_modules(GSETTINGS_QT6 REQUIRED IMPORTED_TARGET gsettings-qt6)

# Repo root, for the appearance helpers shared with gxdm-greeter-neo.
get_filename_component(GXDM_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

# --- Shared DDE widgets and helpers -----------------------------------------
file(GLOB NEO_COMMON_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/view/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/global_util/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/global_util/dbus/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/widgets/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/session_widgets/*.cpp"
)
# Wallpaper, cursor, scale and OS defaults, shared with gxdm-greeter-neo.
list(APPEND NEO_COMMON_SOURCES
    "${GXDM_REPO_ROOT}/src/common/GreeterAppearance.cpp"
    "${GXDM_REPO_ROOT}/src/common/GreeterCursor.cpp"
    "${GXDM_REPO_ROOT}/src/common/GreeterScale.cpp"
    "${GXDM_REPO_ROOT}/src/common/SystemDefaults.cpp"
)
file(GLOB NEO_COMMON_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/view/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/global_util/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/global_util/dbus/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/widgets/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/session_widgets/*.h"
)
list(APPEND NEO_COMMON_HEADERS
    "${GXDM_REPO_ROOT}/src/common/GreeterAppearance.h"
    "${GXDM_REPO_ROOT}/src/common/GreeterCursor.h"
    "${GXDM_REPO_ROOT}/src/common/GreeterScale.h"
    "${GXDM_REPO_ROOT}/src/common/SystemDefaults.h"
)

set(NEO_VENDOR_SOURCES
    vendor/src/backend/users/users.cc
    vendor/src/backend/sessions/sessions.cc
    vendor/src/utils/logger/logger.cc
)
set(NEO_VENDOR_HEADERS
    vendor/src/backend/users/users.h
    vendor/src/backend/sessions/sessions.h
    vendor/src/backend/global_def/global.h
    vendor/src/utils/logger/logger.h
    vendor/src/utils/logger/ansi_escape_code_def.h
)

# Resources used by shared widgets (compiled into each executable).
set(NEO_WIDGET_RESOURCES
    widgets/widgetsimages.qrc
    widgets/widgetstheme.qrc
)

set(NEO_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/global_util"
    "${CMAKE_CURRENT_SOURCE_DIR}/global_util/dbus"
    "${CMAKE_CURRENT_SOURCE_DIR}/widgets"
    "${CMAKE_CURRENT_SOURCE_DIR}/session_widgets"
    "${CMAKE_CURRENT_SOURCE_DIR}/view"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor"
    "${GXDM_REPO_ROOT}/src/common"
)

set(NEO_COMMON_LIBS
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::DBus Qt6::Network Qt6::Xml Qt6::Svg
    PkgConfig::DTK2WIDGET
    PkgConfig::GSETTINGS_QT6
    X11 Xi Xtst Xcursor Xfixes Xrandr Xext
)

# --- Greeter (talks to the GXDM daemon over --socket) ------------------------
add_executable(gxdm-greeter-classic
    main.cpp
    loginwindow.cpp
    loginwindow.h
    greeterworker.cpp
    greeterworker.h
    ${NEO_COMMON_SOURCES}
    ${NEO_COMMON_HEADERS}
    ${NEO_VENDOR_SOURCES}
    ${NEO_VENDOR_HEADERS}
    image.qrc
    logintheme.qrc
    ${NEO_WIDGET_RESOURCES}
)
target_include_directories(gxdm-greeter-classic PRIVATE ${NEO_INCLUDE_DIRS})
target_link_libraries(gxdm-greeter-classic PRIVATE ${NEO_COMMON_LIBS})
install(TARGETS gxdm-greeter-classic DESTINATION "${CMAKE_INSTALL_BINDIR}")
