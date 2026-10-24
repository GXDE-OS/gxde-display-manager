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
  property var sessions: []
  property int selectedIndex: -1
  property real maximumHeight: 320
  property int currentIndex: -1

  readonly property color highlightColor: light ? "#0081ff" : "#024cca"
  readonly property color highlightedTextColor: light ? "#ffffff" : "#f1f6ff"
  readonly property color textColor: light ? "#000000" : "#ffffff"
  readonly property color secondaryTextColor: light ? "#99000000" : "#99ffffff"
  readonly property int itemHeight: 30
  readonly property int itemMargin: 6
  readonly property int itemRadius: 6
  readonly property int textIndent: 30
  readonly property int iconSize: 16
  readonly property int iconSpacing: 8
  readonly property string iconPrefix: "qrc:/icons/sessions/"

  signal selected(int index)

  function iconFor(session) {
    if (!session) {
      return ""
    }
    const key = (String(session.file || "").replace(/\.desktop$/, "") + " "
      + String(session.name || "")).toLowerCase()
    if (key.indexOf("gxde") !== -1 || key.indexOf("deepin") !== -1) {
      return menu.iconPrefix + "gxde.svg"
    }
    const known = ["gnome", "plasma", "kde", "xfce", "ubuntu", "fluxbox",
      "lxde", "enlightenment"]
    for (const name of known) {
      if (key.indexOf(name) !== -1) {
        return menu.iconPrefix + (name === "kde" ? "plasma" : name) + ".svg"
      }
    }
    return menu.iconPrefix
      + (session.type === "wayland" ? "wayland" : "x11") + ".svg"
  }

  function typeLabel(index) {
    const session = menu.sessions[index]
    for (let i = 0; i < menu.sessions.length; ++i) {
      if (i !== index && menu.sessions[i].name === session.name) {
        return session.type === "wayland" ? "Wayland" : "X11"
      }
    }
    return ""
  }

  function step(direction) {
    const count = menu.sessions.length
    if (count === 0) {
      return
    }
    const from = menu.currentIndex >= 0 ? menu.currentIndex
      : menu.selectedIndex >= 0 ? menu.selectedIndex
      : direction > 0 ? -1 : 0
    menu.currentIndex = (from + direction + count) % count
    list.positionViewAtIndex(menu.currentIndex, ListView.Contain)
  }

  function choose(index) {
    if (index < 0 || index >= menu.sessions.length) {
      return
    }
    menu.close()
    menu.selected(index)
  }

  implicitWidth: Math.max(162, list.widest)
  height: Math.min(maximumHeight,
    list.contentHeight + topPadding + bottomPadding)
  padding: 0
  topPadding: 8
  bottomPadding: 8
  modal: false
  focus: true
  transformOrigin: Popup.BottomRight
  closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

  onAboutToShow: menu.currentIndex = -1
  onOpened: list.positionViewAtIndex(Math.max(0, menu.selectedIndex),
    ListView.Contain)

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

  contentItem: ListView {
    id: list

    property real widest: 0

    objectName: "sessionList"
    clip: true
    focus: true
    model: menu.sessions
    boundsBehavior: Flickable.StopAtBounds
    keyNavigationEnabled: false
    highlightFollowsCurrentItem: false
    ScrollBar.vertical: ScrollBar {
      policy: list.contentHeight > list.height
        ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
    }

    Keys.onPressed: (event) => {
      if (event.key === Qt.Key_Down || event.key === Qt.Key_Tab) {
        menu.step(1)
      } else if (event.key === Qt.Key_Up || event.key === Qt.Key_Backtab) {
        menu.step(-1)
      } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter
          || event.key === Qt.Key_Space) {
        menu.choose(menu.currentIndex)
      } else {
        return
      }
      event.accepted = true
    }

    Rectangle {
      id: highlight

      property int shownIndex: -1

      z: -1
      x: menu.itemMargin
      width: list.width - menu.itemMargin * 2
      height: menu.itemHeight
      radius: menu.itemRadius
      color: menu.highlightColor
      visible: menu.currentIndex >= 0

      Connections {
        target: menu

        function onCurrentIndexChanged() {
          const index = menu.currentIndex
          if (index >= 0) {
            const to = index * menu.itemHeight
            if (highlight.shownIndex >= 0) {
              slide.to = to
              slide.restart()
            } else {
              slide.stop()
              highlight.y = to
            }
          }
          highlight.shownIndex = index
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

    delegate: Item {
      id: entry

      required property var modelData
      required property int index
      readonly property bool current: menu.currentIndex === index

      width: list.width
      height: menu.itemHeight

      Component.onCompleted: list.widest = Math.max(list.widest,
        label.x + label.implicitWidth + (typeText.text
          ? 16 + typeText.implicitWidth + 16 : menu.textIndent))

      Image {
        x: 9
        anchors.verticalCenter: parent.verticalCenter
        width: 14
        height: 12
        sourceSize: Qt.size(14, 12)
        visible: entry.index === menu.selectedIndex
        source: menu.light && !entry.current
          ? "qrc:/icons/menu-check-dark.svg" : "qrc:/icons/menu-check.svg"
      }

      Image {
        id: icon

        x: menu.textIndent
        anchors.verticalCenter: parent.verticalCenter
        width: menu.iconSize
        height: menu.iconSize
        sourceSize: Qt.size(menu.iconSize, menu.iconSize)
        source: menu.iconFor(entry.modelData)
      }

      Text {
        id: label

        x: icon.x + menu.iconSize + menu.iconSpacing
        anchors.verticalCenter: parent.verticalCenter
        width: Math.min(implicitWidth, (typeText.text
          ? typeText.x - 16 : parent.width - menu.textIndent) - x)
        text: entry.modelData.name
        elide: Text.ElideRight
        color: entry.current ? menu.highlightedTextColor : menu.textColor
        font.pixelSize: 14
        renderType: Text.NativeRendering
      }

      Text {
        id: typeText

        anchors.right: parent.right
        anchors.rightMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        text: menu.typeLabel(entry.index)
        color: entry.current ? menu.highlightedTextColor
          : menu.secondaryTextColor
        font.pixelSize: 12
        renderType: Text.NativeRendering
      }

      MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onEntered: menu.currentIndex = entry.index
        onExited: {
          if (menu.currentIndex === entry.index) {
            menu.currentIndex = -1
          }
        }
        onClicked: menu.choose(entry.index)
      }
    }
  }
}
