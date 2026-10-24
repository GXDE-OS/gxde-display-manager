# Copyright (C) 2026 CharOfString <root@charofstring.cc>
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation, version 3.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# this program. If not, see <https://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.21)

project(gxde-greeter VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5
  REQUIRED COMPONENTS
    Quick
    Network
)

qt_standard_project_setup(REQUIRES 6.5)

foreach(cur IN ITEMS
  qml/LiquidGlass.qml
  qml/LiquidGlassButton.qml
  qml/Main.qml
  qml/TintArea.qml
)
get_filename_component(qml_alias "${cur}" NAME)
set_source_files_properties("${cur}" PROPERTIES QT_RESOURCE_ALIAS "${qml_alias}")
endforeach()

qt_add_resources(GREETER_RESOURCES
  res/resources.qrc
)

qt_add_executable(gxde-greeter
  ${GREETER_RESOURCES}
  src/session_list.cc
  src/session_list.h
  src/main.cc
  src/power_backend.cc
  src/power_backend.h
  src/utils/macros.h
)

set_source_files_properties(qml/MainWindow.qml PROPERTIES
  QT_RESOURCE_ALIAS MainWindow.qml)
set_source_files_properties(qml/components/UserSelecter.qml PROPERTIES
  QT_RESOURCE_ALIAS UserSelecter.qml)
set_source_files_properties(qml/components/PasswordCard.qml PROPERTIES
  QT_RESOURCE_ALIAS PasswordCard.qml)
set_source_files_properties(qml/components/UserAvatar.qml PROPERTIES
  QT_RESOURCE_ALIAS UserAvatar.qml)
set_source_files_properties(qml/components/VirtualKeyboardPanel.qml PROPERTIES
  QT_RESOURCE_ALIAS VirtualKeyboardPanel.qml)
set_source_files_properties(qml/components/DtkFrame.qml PROPERTIES
  QT_RESOURCE_ALIAS DtkFrame.qml)
set_source_files_properties(qml/components/PowerMenu.qml PROPERTIES
  QT_RESOURCE_ALIAS PowerMenu.qml)

set_source_files_properties(qml/components/SessionMenu.qml PROPERTIES
  QT_RESOURCE_ALIAS SessionMenu.qml)

qt_add_qml_module(gxde-greeter
  URI Greeter
  VERSION 1.0
  QML_FILES
    qml/MainWindow.qml
    qml/components/UserSelecter.qml
    qml/components/PasswordCard.qml
    qml/components/UserAvatar.qml
    qml/components/VirtualKeyboardPanel.qml
    qml/components/DtkFrame.qml
    qml/components/SessionMenu.qml
    qml/components/PowerMenu.qml
)

target_include_directories(gxde-greeter PRIVATE ..)

target_link_libraries(gxde-greeter PRIVATE
  Qt6::Quick
  Qt6::Network
)

include(GNUInstallDirs)

install(TARGETS gxde-greeter
  BUNDLE DESTINATION .
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

option(GREETER_BUILD_TESTS "Build greeter backend tests" OFF)
if(GREETER_BUILD_TESTS)
  enable_testing()
  find_package(Qt6 REQUIRED COMPONENTS Test)
  qt_add_executable(powerbackend-test
    tests/power_backend_test.cc
    src/power_backend.cc
    src/power_backend.h
  )
  target_include_directories(powerbackend-test PRIVATE ..)
  target_link_libraries(powerbackend-test PRIVATE Qt6::Network Qt6::Test)
  add_test(NAME powerbackend COMMAND powerbackend-test)
  qt_add_executable(session-list-test
    tests/session_list_test.cc
    src/session_list.cc
    src/session_list.h
  )
  target_include_directories(session-list-test PRIVATE ..)
  target_link_libraries(session-list-test PRIVATE Qt6::Test)
  add_test(NAME session-list COMMAND session-list-test)
  get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
  execute_process(COMMAND "${QT_QMAKE_EXECUTABLE}" -query QT_INSTALL_BINS
    OUTPUT_VARIABLE QT_BIN_DIRECTORY OUTPUT_STRIP_TRAILING_WHITESPACE)
  find_program(QML6_TEST_RUNNER qmltestrunner
    PATHS "${QT_BIN_DIRECTORY}" NO_DEFAULT_PATH)
  if(QML6_TEST_RUNNER)
    add_test(NAME session-menu COMMAND "${QML6_TEST_RUNNER}"
      -platform offscreen -input "${CMAKE_CURRENT_SOURCE_DIR}/tests/qml")
  endif()
endif()
