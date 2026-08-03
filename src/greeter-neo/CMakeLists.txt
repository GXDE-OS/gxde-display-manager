cmake_minimum_required(VERSION 3.16)
project(gxdm-greeter-neo LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 COMPONENTS Core Gui Widgets DBus Network Xml Svg Qml REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(DTK2WIDGET REQUIRED IMPORTED_TARGET dtk2widget)

# Repo root: the lock reuses GXDM's own authenticator (src/auth + src/common).
get_filename_component(GXDM_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

# --- Sources shared by the greeter and the lock screen -----------------------
file(GLOB NEO_COMMON_SOURCES
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
    X11 Xi Xtst Xcursor Xfixes Xrandr Xext
)

# --- Greeter (talks to the GXDM daemon over --socket) ------------------------
add_executable(gxdm-greeter-neo
    main.cpp
    loginwindow.cpp
    greeterworker.cpp
    ${NEO_COMMON_SOURCES}
    ${NEO_VENDOR}
    image.qrc
    logintheme.qrc
    ${NEO_WIDGET_RESOURCES}
)
target_include_directories(gxdm-greeter-neo PRIVATE ${NEO_INCLUDE_DIRS})
target_link_libraries(gxdm-greeter-neo PRIVATE ${NEO_COMMON_LIBS})
install(TARGETS gxdm-greeter-neo DESTINATION "${CMAKE_INSTALL_BINDIR}")

# --- GXDM authenticator (src/auth + src/common) reused by the lock -----------
# Generate Constants.h (Auth.cpp needs LIBEXEC_INSTALL_DIR -> gxdm-helper path).
# Values match the deployed layout under /usr.
set(CMAKE_INSTALL_FULL_BINDIR     "/usr/bin")
set(CMAKE_INSTALL_FULL_LIBEXECDIR "/usr/libexec")
set(CMAKE_INSTALL_FULL_SYSCONFDIR "/etc")
set(DATA_INSTALL_DIR              "/usr/share/gxdm")
set(QML_INSTALL_DIR               "/usr/share/gxdm/qml")
set(COMPONENTS_TRANSLATION_DIR    "/usr/share/gxdm/translations")
set(RUNTIME_DIR                   "/run/gxdm")
set(STATE_DIR                     "/var/lib/gxdm")
set(ACCOUNTSSERVICE_DATA_DIR      "/var/lib/AccountsService")
set(SESSION_COMMAND               "/usr/share/gxdm/scripts/Xsession")
set(WAYLAND_SESSION_COMMAND       "/usr/share/gxdm/scripts/wayland-session")
set(CONFIG_FILE                   "/etc/gxdm.conf")
set(CONFIG_DIR                    "/etc/gxdm.conf.d")
set(SYSTEM_CONFIG_DIR             "/usr/lib/gxdm/gxdm.conf.d")
set(LOG_FILE                      "/var/log/gxdm.log")
set(UID_MIN 1000)
set(UID_MAX 60000)
set(HALT_COMMAND   "/usr/bin/systemctl poweroff")
set(REBOOT_COMMAND "/usr/bin/systemctl reboot")
configure_file("${GXDM_REPO_ROOT}/src/common/Constants.h.in"
               "${CMAKE_CURRENT_BINARY_DIR}/gen/Constants.h" @ONLY)

set(GXDM_AUTH_SOURCES
    "${GXDM_REPO_ROOT}/src/auth/Auth.cpp"
    "${GXDM_REPO_ROOT}/src/auth/AuthPrompt.cpp"
    "${GXDM_REPO_ROOT}/src/auth/AuthRequest.cpp"
    "${GXDM_REPO_ROOT}/src/common/SafeDataStream.cpp"
)

# --- Lock screen (in-session, reuses GXDM's SDDM::Auth in auth-only mode) -----
file(GLOB NEO_LOCK_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/lock/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/lock/dbus/*.cpp"
)
add_executable(gxdm-lock-neo
    ${NEO_LOCK_SOURCES}
    ${NEO_COMMON_SOURCES}
    ${NEO_VENDOR}
    ${GXDM_AUTH_SOURCES}
    lock/images.qrc
    lock/logintheme.qrc
    ${NEO_WIDGET_RESOURCES}
)
target_include_directories(gxdm-lock-neo PRIVATE
    ${NEO_INCLUDE_DIRS}
    "${CMAKE_CURRENT_SOURCE_DIR}/lock"
    "${GXDM_REPO_ROOT}/src/auth"
    "${GXDM_REPO_ROOT}/src/common"
    "${CMAKE_CURRENT_BINARY_DIR}/gen"
)
target_link_libraries(gxdm-lock-neo PRIVATE
    ${NEO_COMMON_LIBS}
    Qt6::Qml
)
install(TARGETS gxdm-lock-neo DESTINATION "${CMAKE_INSTALL_BINDIR}")
