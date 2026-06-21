"""Install-path constants for gxdm
"""

# Multiarch
MULTIARCH = "x86_64-linux-gnu"

CONSTANTS = {
    "CMAKE_INSTALL_FULL_BINDIR": "/usr/bin",
    "CMAKE_INSTALL_FULL_LIBEXECDIR": "/usr/libexec",
    "CMAKE_INSTALL_FULL_SYSCONFDIR": "/etc",
    "BIN_INSTALL_DIR": "/usr/bin",
    "LIBEXEC_INSTALL_DIR": "/usr/libexec",
    "DATA_INSTALL_DIR": "/usr/share/gxdm",
    "SYS_CONFIG_DIR": "/etc",
    "QML_INSTALL_DIR": "/usr/lib/" + MULTIARCH + "/qt6/qml",
    "IMPORTS_INSTALL_DIR": "/usr/lib/" + MULTIARCH + "/qt6/qml",
    "COMPONENTS_TRANSLATION_DIR": "/usr/share/gxdm/translations-qt6",
    "RUNTIME_DIR": "/run/gxdm",
    "STATE_DIR": "/var/lib/gxdm",
    "ACCOUNTSSERVICE_DATA_DIR": "/var/lib/AccountsService",
    "SESSION_COMMAND": "/usr/share/gxdm/scripts/Xsession",
    "WAYLAND_SESSION_COMMAND": "/usr/share/gxdm/scripts/wayland-session",
    "CONFIG_FILE": "/etc/gxdm.conf",
    "CONFIG_DIR": "/etc/gxdm.conf.d",
    "SYSTEM_CONFIG_DIR": "/usr/lib/gxdm/gxdm.conf.d",
    "LOG_FILE": "/var/log/gxdm.log",
    "UID_MIN": "1000",
    "UID_MAX": "60000",
    "HALT_COMMAND": "",
    "REBOOT_COMMAND": "",
    "SDDM_INITIAL_VT": "1",
    "SDDM_VERSION_STRING": "0.21.0",
}
