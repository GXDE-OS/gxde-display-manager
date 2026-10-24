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

Popup {
  id: menu

  property bool light: false
  property Item backdrop: null
  property var availableActions: ["reboot", "shutdown", "suspend"]
  property int originCorner: Item.BottomRight
  property int currentIndex: -1

  readonly property var actions: [
    { action: "reboot", text: qsTr("重启") },
    { action: "shutdown", text: qsTr("关机") },
    { separator: true },
    { action: "suspend", text: qsTr("待机") }
  ]

  readonly property color highlightColor: light ? "#0081ff" : "#024cca"
  readonly property color highlightedTextColor: light ? "#ffffff" : "#f1f6ff"
  readonly property color textColor: light ? "#000000" : "#ffffff"
  readonly property color separatorColor: light ? "#1a000000" : "#0dffffff"
  readonly property int itemHeight: 30
  readonly property int itemMargin: 6
  readonly property int itemRadius: 6
  readonly property int textIndent: 30

  signal triggered(string action)

  function isEnabled(index) {
    const entry = menu.actions[index]
    return !entry.separator
      && menu.availableActions.indexOf(entry.action) !== -1
  }

  function step(direction) {
    const count = menu.actions.length
    let index = menu.currentIndex
    for (let i = 0; i < count; ++i) {
      index = index < 0
        ? (direction > 0 ? 0 : count - 1)
        : (index + direction + count) % count
      if (menu.isEnabled(index)) {
        menu.currentIndex = index
        return
      }
    }
  }

  function trigger(index) {
    if (!menu.isEnabled(index)) {
      return
    }
    const action = menu.actions[index].action
    menu.close()
    menu.triggered(action)
  }

  padding: 0
  topPadding: 8
  bottomPadding: 8
  modal: false
  focus: true
  transformOrigin: originCorner === Item.BottomRight ? Popup.BottomRight
    : originCorner === Item.BottomLeft ? Popup.BottomLeft
    : originCorner === Item.TopLeft ? Popup.TopLeft
    : Popup.TopRight
  closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

  onAboutToShow: menu.currentIndex = -1

  enter: Transition {
    ParallelAnimation {
      NumberAnimation {
        property: "opacity"
        from: 0.0
        to: 1.0
        duration: 90
        easing.type: Easing.OutCubic
      }
      NumberAnimation {
        property: "scale"
        from: 0.95
        to: 1.0
        duration: 90
        easing.type: Easing.OutCubic
      }
    }
  }

  exit: Transition {
    NumberAnimation {
      property: "opacity"
      to: 0.0
      duration: 90
      easing.type: Easing.OutCubic
    }
  }

  background: DtkFrame {
    light: menu.light
    backdrop: menu.backdrop
    trackedGeometry: [menu.x, menu.y, menu.scale, menu.opened]
  }

  contentItem: Item {
    implicitWidth: Math.max(162, column.implicitWidth)
    implicitHeight: column.implicitHeight
    focus: true

    Keys.onPressed: (event) => {
      if (event.key === Qt.Key_Down || event.key === Qt.Key_Tab) {
        menu.step(1)
      } else if (event.key === Qt.Key_Up || event.key === Qt.Key_Backtab) {
        menu.step(-1)
      } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter
          || event.key === Qt.Key_Space) {
        menu.trigger(menu.currentIndex)
      } else {
        return
      }
      event.accepted = true
    }

    Rectangle {
      id: highlight

      property Item target: menu.currentIndex >= 0
        ? repeater.itemAt(menu.currentIndex) : null

      x: menu.itemMargin
      width: parent.width - menu.itemMargin * 2
      height: menu.itemHeight
      radius: menu.itemRadius
      color: menu.highlightColor
      visible: target !== null

      onTargetChanged: {
        if (!target) {
          return
        }
        if (visible) {
          slide.to = target.y
          slide.restart()
        } else {
          slide.stop()
          y = target.y
        }
      }

      NumberAnimation {
        id: slide

        target: highlight
        property: "y"
        duration: 100
        easing.type: Easing.OutCubic
      }
    }

    Column {
      id: column

      width: parent.width

      Repeater {
        id: repeater

        model: menu.actions

        delegate: Item {
          id: entry

          required property var modelData
          required property int index
          readonly property bool current: menu.currentIndex === index

          width: column.width
          implicitWidth: label.implicitWidth + menu.textIndent * 2
          height: modelData.separator ? 2 : menu.itemHeight

          Rectangle {
            anchors.fill: parent
            anchors.leftMargin: menu.itemMargin
            anchors.rightMargin: menu.itemMargin
            visible: entry.modelData.separator === true
            color: menu.separatorColor
          }

          Text {
            id: label

            anchors.verticalCenter: parent.verticalCenter
            x: menu.textIndent
            visible: !entry.modelData.separator
            text: entry.modelData.text || ""
            color: entry.current ? menu.highlightedTextColor : menu.textColor
            opacity: menu.isEnabled(entry.index) ? 1.0 : 0.4
            font.pixelSize: 14
            renderType: Text.NativeRendering
          }

          MouseArea {
            anchors.fill: parent
            enabled: menu.isEnabled(entry.index)
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onEntered: menu.currentIndex = entry.index
            onExited: {
              if (menu.currentIndex === entry.index) {
                menu.currentIndex = -1
              }
            }
            onClicked: menu.trigger(entry.index)
          }
        }
      }
    }
  }
}
