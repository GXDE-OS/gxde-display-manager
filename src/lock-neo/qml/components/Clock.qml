// Copyright (C) 2026 CharOfString <root@charofstring.cc>
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

Column {
  id: clock

  property bool primaryScreen: true
  property date now: new Date()

  spacing: 2

  Timer {
    interval: 1000
    running: true
    repeat: true
    onTriggered: clock.now = new Date()
  }

  Text {
    anchors.horizontalCenter: parent.horizontalCenter
    text: Qt.formatTime(clock.now, "HH:mm")
    color: "#f5f8ff"
    font.pixelSize: {
      const base = clock.parent ? clock.parent.height : 720
      return Math.max(44, Math.min(104, Math.round(base * 0.09)))
    }
    font.weight: Font.Light
    font.letterSpacing: -1
    renderType: Text.NativeRendering
  }

  Text {
    anchors.horizontalCenter: parent.horizontalCenter
    text: Qt.formatDate(clock.now, Qt.SystemLocaleLongDate)
    color: clock.primaryScreen ? "#d9e2f0" : "#b3c0d2"
    font.pixelSize: 16
    renderType: Text.NativeRendering
  }
}
