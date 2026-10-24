// Copyright (C) CharOfString <root@charofstring.cc>
// 
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation, version 3.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program. If not, see <https://www.gnu.org/licenses/>.

import QtQuick
import QtQuick.Controls
import QtTest
import "../../qml/components"

Item {
  width: 640
  height: 480

  SessionMenu {
    id: menu
    sessions: [
      { name: "GXDE", type: "x11" },
      { name: "Deepin", type: "wayland" }
    ]
    selectedIndex: 0
  }

  SignalSpy {
    id: selection
    target: menu
    signalName: "selected"
  }

  TestCase {
    name: "SessionMenu"
    when: windowShown

    function init() {
      selection.clear()
      menu.open()
      tryCompare(menu, "opened", true)
    }

    function cleanup() {
      menu.close()
      tryCompare(menu, "visible", false)
    }

    function test_keyboard() {
      keyClick(Qt.Key_Down)
      keyClick(Qt.Key_Return)
      compare(selection.count, 1)
      compare(selection.signalArguments[0][0], 1)
    }

    function test_wrap() {
      keyClick(Qt.Key_Up)
      keyClick(Qt.Key_Return)
      compare(selection.signalArguments[0][0], 1)
    }

    function test_cancel() {
      keyClick(Qt.Key_Down)
      keyClick(Qt.Key_Escape)
      tryCompare(menu, "visible", false)
      compare(selection.count, 0)
    }

    function test_mouse() {
      mouseClick(menu.contentItem, 80, 55)
      compare(selection.count, 1)
      compare(selection.signalArguments[0][0], 1)
    }
  }
}
