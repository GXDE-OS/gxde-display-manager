install(DIRECTORY "flags" DESTINATION "${DATA_INSTALL_DIR}")

install(FILES
    "org.freedesktop.DisplayManager.conf"
    DESTINATION "${DBUS_CONFIG_DIR}"
    RENAME ${DBUS_CONFIG_FILENAME}
)

install(FILES
    "top.gxde.DisplayManager.conf"
    DESTINATION "${DBUS_CONFIG_DIR}"
)

configure_file(
    "top.gxde.DisplayManager.service.in"
    "${CMAKE_CURRENT_BINARY_DIR}/top.gxde.DisplayManager.service"
    @ONLY
)
configure_file(
    "gxdm-lock-neo.desktop.in"
    "${CMAKE_CURRENT_BINARY_DIR}/gxdm-lock-neo.desktop"
    @ONLY
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/top.gxde.DisplayManager.service"
    DESTINATION "${CMAKE_INSTALL_FULL_DATADIR}/dbus-1/services"
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/gxdm-lock-neo.desktop"
    DESTINATION "${CMAKE_INSTALL_SYSCONFDIR}/xdg/autostart"
)

install(FILES
    "faces/root.face.icon.png"
    DESTINATION "${DATA_INSTALL_DIR}/faces"
    RENAME "root.face.icon"
)

install(FILES
    "faces/default.face.icon.png"
    DESTINATION "${DATA_INSTALL_DIR}/faces"
    RENAME ".face.icon"
)

install(FILES
    "scripts/Xsession"
    "scripts/Xsetup"
    "scripts/Xstop"
    "scripts/wayland-session"
    DESTINATION "${DATA_INSTALL_DIR}/scripts"
    PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE
)

if(BUILD_MAN_PAGES)
    add_subdirectory(man)
endif()
add_subdirectory(themes)
add_subdirectory(translations)
