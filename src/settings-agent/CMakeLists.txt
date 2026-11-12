# Session-bus top.gxde.DisplayManager: forwards greeter settings to the
# system daemon. D-Bus activated (data/top.gxde.DisplayManager.service.in).
add_executable(gxdm-settings-agent
    main.cpp
    SettingsAgent.cpp
    SettingsAgent.h
)
target_link_libraries(gxdm-settings-agent
    Qt${QT_MAJOR_VERSION}::Core
    Qt${QT_MAJOR_VERSION}::DBus
)
install(TARGETS gxdm-settings-agent RUNTIME DESTINATION "${CMAKE_INSTALL_LIBEXECDIR}")
