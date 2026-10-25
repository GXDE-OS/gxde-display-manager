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
  width: 800
  height: 600

  TextField {
    id: field
    echoMode: TextInput.Password
    inputMethodHints: Qt.ImhHiddenText | Qt.ImhNoPredictiveText
  }

  VirtualKeyboardPanel {
    id: panel
    width: parent.width
    anchors.bottom: parent.bottom
  }

  TestCase {
    name: "VirtualKeyboard"
    when: windowShown

    function test_passwordInput() {
      field.forceActiveFocus()
      tryCompare(field, "activeFocus", true)
      let key = null
      tryVerify(function() {
        key = findChild(panel, "virtualKeyboardKeyQ")
        return key !== null && key.width > 0 && key.height > 0
      })
      mouseClick(key, key.width / 2, key.height / 2)
      tryCompare(field, "text", "q")
      verify(field.activeFocus)
      panel.visible = false
      compare(field.text, "q")
      panel.visible = true
      mouseClick(key, key.width / 2, key.height / 2)
      tryCompare(field, "text", "qq")
    }
  }
}
