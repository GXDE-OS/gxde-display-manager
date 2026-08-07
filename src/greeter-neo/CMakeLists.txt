cmake_minimum_required(VERSION 3.16)
project(gxdm-greeter-neo LANGUAGES CXX)

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

find_package(Qt6 COMPONENTS Core Gui Widgets DBus Network Xml Svg Qml REQUIRED)
find_package(LayerShellQt REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(DTK2WIDGET REQUIRED IMPORTED_TARGET dtk2widget)
pkg_check_modules(GSETTINGS_QT6 REQUIRED IMPORTED_TARGET gsettings-qt6)

# Repo root: the lock reuses GXDM's own authenticator (src/auth + src/common).
get_filename_component(GXDM_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

# --- Sources shared by the greeter and the lock screen -----------------------
file(GLOB NEO_COMMON_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/view/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/global_util/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/global_util/dbus/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/widgets/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/session_widgets/*.cpp"
)
file(GLOB NEO_COMMON_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/view/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/global_util/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/global_util/dbus/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/widgets/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/session_widgets/*.h"
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
)

set(NEO_COMMON_LIBS
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::DBus Qt6::Network Qt6::Xml Qt6::Svg
    PkgConfig::DTK2WIDGET
    PkgConfig::GSETTINGS_QT6
    X11 Xi Xtst Xcursor Xfixes Xrandr Xext
)

# --- Greeter (talks to the GXDM daemon over --socket) ------------------------
add_executable(gxdm-greeter-neo
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
target_include_directories(gxdm-greeter-neo PRIVATE ${NEO_INCLUDE_DIRS})
target_link_libraries(gxdm-greeter-neo PRIVATE ${NEO_COMMON_LIBS})
install(TARGETS gxdm-greeter-neo DESTINATION "${CMAKE_INSTALL_BINDIR}")

# --- GXDM authenticator (src/auth + src/common) reused by the lock -----------
# The top-level build already generated Constants.h with its configured paths.
# Keep this directory independently buildable for greeter development as well.
if(CMAKE_SOURCE_DIR STREQUAL GXDM_REPO_ROOT)
    set(GXDM_CONSTANTS_INCLUDE_DIR "${CMAKE_BINARY_DIR}/src/common")
else()
    set(DATA_INSTALL_DIR              "${CMAKE_INSTALL_FULL_DATADIR}/gxdm")
    set(QML_INSTALL_DIR               "${DATA_INSTALL_DIR}/qml")
    set(COMPONENTS_TRANSLATION_DIR    "${DATA_INSTALL_DIR}/translations-qt6")
    set(RUNTIME_DIR                   "/run/gxdm")
    set(STATE_DIR                     "${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/lib/gxdm")
    set(ACCOUNTSSERVICE_DATA_DIR      "/var/lib/AccountsService")
    set(SESSION_COMMAND               "${DATA_INSTALL_DIR}/scripts/Xsession")
    set(WAYLAND_SESSION_COMMAND       "${DATA_INSTALL_DIR}/scripts/wayland-session")
    set(CONFIG_FILE                   "${CMAKE_INSTALL_FULL_SYSCONFDIR}/gxdm.conf")
    set(CONFIG_DIR                    "${CMAKE_INSTALL_FULL_SYSCONFDIR}/gxdm.conf.d")
    set(SYSTEM_CONFIG_DIR             "${CMAKE_INSTALL_PREFIX}/lib/gxdm/gxdm.conf.d")
    set(LOG_FILE                      "${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/log/gxdm.log")
    set(UID_MIN 1000)
    set(UID_MAX 60000)
    set(HALT_COMMAND   "/usr/bin/systemctl poweroff")
    set(REBOOT_COMMAND "/usr/bin/systemctl reboot")
    configure_file("${GXDM_REPO_ROOT}/src/common/Constants.h.in"
                   "${CMAKE_CURRENT_BINARY_DIR}/gen/Constants.h" @ONLY)
    set(GXDM_CONSTANTS_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/gen")
endif()

set(GXDM_AUTH_FILES
    "${GXDM_REPO_ROOT}/src/auth/Auth.cpp"
    "${GXDM_REPO_ROOT}/src/auth/Auth.h"
    "${GXDM_REPO_ROOT}/src/auth/AuthMessages.h"
    "${GXDM_REPO_ROOT}/src/auth/AuthPrompt.cpp"
    "${GXDM_REPO_ROOT}/src/auth/AuthPrompt.h"
    "${GXDM_REPO_ROOT}/src/auth/AuthRequest.cpp"
    "${GXDM_REPO_ROOT}/src/auth/AuthRequest.h"
    "${GXDM_REPO_ROOT}/src/common/MessageHandler.h"
    "${GXDM_REPO_ROOT}/src/common/SafeDataStream.cpp"
    "${GXDM_REPO_ROOT}/src/common/SafeDataStream.h"
)

# --- Lock screen (in-session, reuses GXDM's SDDM::Auth in auth-only mode) -----
file(GLOB NEO_LOCK_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/lock/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/lock/dbus/*.cpp"
)
file(GLOB NEO_LOCK_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/lock/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/lock/dbus/*.h"
)
add_executable(gxdm-lock-neo
    ${NEO_LOCK_SOURCES}
    ${NEO_LOCK_HEADERS}
    ${NEO_COMMON_SOURCES}
    ${NEO_COMMON_HEADERS}
    ${NEO_VENDOR_SOURCES}
    ${NEO_VENDOR_HEADERS}
    ${GXDM_AUTH_FILES}
    lock/images.qrc
    lock/logintheme.qrc
    ${NEO_WIDGET_RESOURCES}
)
target_include_directories(gxdm-lock-neo PRIVATE
    ${NEO_INCLUDE_DIRS}
    "${CMAKE_CURRENT_SOURCE_DIR}/lock"
    "${GXDM_REPO_ROOT}/src/auth"
    "${GXDM_REPO_ROOT}/src/common"
    "${GXDM_CONSTANTS_INCLUDE_DIR}"
)
target_link_libraries(gxdm-lock-neo PRIVATE
    ${NEO_COMMON_LIBS}
    LayerShellQt::Interface
    Qt6::Qml
)
install(TARGETS gxdm-lock-neo DESTINATION "${CMAKE_INSTALL_BINDIR}")
