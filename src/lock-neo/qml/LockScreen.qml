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
import QtQuick.Effects

Item {
  id: screen

  property var backend: null
  property bool primaryScreen: true
  property bool virtualKeyboardAvailable: false
  property bool testMode: false
  property bool windowVisible: true
  readonly property bool ready: backend !== null

  // Lock anim: blur strength graduately applies, and 
  property real revealProgress: 0
  readonly property real wallpaperBlurMax: 1.0
  property bool unlocking: false

  signal requestQuit()
  signal unlockFadeFinished()

  function startReveal() {
    if (!screen.windowVisible || wallpaper.status !== Image.Ready) return
    revealAnimation.restart()
  }

  function resetReveal() {
    revealAnimation.stop()
    unlockFadeAnimation.stop()
    screen.revealProgress = 0
    screen.unlocking = false
    screen.opacity = 1
  }

  function cycleLayout() {
    if (!screen.backend) return
    const layouts = screen.backend.kbLayouts
    if (!layouts || layouts.length < 2) return
    const current = layouts.indexOf(screen.backend.currentKbLayout)
    screen.backend.SetKbLayout(layouts[(current + 1) % layouts.length])
  }

  function requestPower(action) {
    if (!screen.backend) return
    screen.backend.RequestPower(action)
    loginCard.focusPassword()
  }

  function handleAuthenticated() {
    loginCard.showSuccess()
    screen.unlocking = true
    unlockFadeAnimation.restart()
  }

  function handleAuthenticationFailed(message) {
    loginCard.showError(message)
  }

  function handlePowerActionStarted(action) {
    loginCard.showPowerProgress(action)
  }

  function handleShowUserList() {
    if (screen.primaryScreen && screen.backend
        && screen.backend.userSwitchAvailable) {
      userSwitcher.open()
    }
  }

  Shortcut {
    sequence: "Escape"
    enabled: screen.testMode
    onActivated: screen.requestQuit()
  }

  onVisibleChanged: {
    if (visible && screen.windowVisible && screen.primaryScreen) {
      loginCard.focusPassword()
    }
  }

  onWindowVisibleChanged: {
    if (windowVisible) {
      screen.startReveal()
      if (screen.primaryScreen) loginCard.focusPassword()
    } else {
      screen.resetReveal()
      loginCard.clearPassword()
    }
  }

  Component.onCompleted: screen.startReveal()

  NumberAnimation {
    id: revealAnimation

    target: screen
    property: "revealProgress"
    from: 0
    to: 1
    duration: 700
    easing.type: Easing.OutCubic
  }

  NumberAnimation {
    id: unlockFadeAnimation

    target: screen
    property: "opacity"
    to: 0
    duration: 450
    easing.type: Easing.InOutQuad
    onFinished: screen.unlockFadeFinished()
  }

  // --- Wallpaper -------------------------------------------------------------

  Rectangle {
    anchors.fill: parent
    gradient: Gradient {
      GradientStop { position: 0.0; color: "#18202b" }
      GradientStop { position: 0.55; color: "#10151c" }
      GradientStop { position: 1.0; color: "#07090c" }
    }
  }

  Image {
    id: wallpaper

    anchors.fill: parent
    source: screen.ready ? screen.backend.backgroundUri : ""
    fillMode: Image.PreserveAspectCrop
    asynchronous: true
    cache: false
    smooth: true
    mipmap: true
    onStatusChanged: if (status === Image.Ready) screen.startReveal()
    visible: source.toString() !== "" && status === Image.Ready
    opacity: screen.revealProgress
    layer.enabled: visible
    layer.smooth: true
    layer.effect: MultiEffect {
      blurEnabled: true
      blurMax: 32
      blur: screen.wallpaperBlurMax * screen.revealProgress
      brightness: -0.22
      saturation: 0.9
      contrast: 0.02
    }
  }

  Rectangle {
    anchors.fill: parent
    color: "#33000000"
  }

  Rectangle {
    anchors.fill: parent
    gradient: Gradient {
      GradientStop { position: 0.0; color: "#66000000" }
      GradientStop { position: 0.45; color: "#1f000000" }
      GradientStop { position: 1.0; color: "#a6000000" }
    }
  }

  // --- Clock -----------------------------------------------------------------

  Clock {
    id: clock

    anchors.horizontalCenter: parent.horizontalCenter
    anchors.top: parent.top
    anchors.topMargin: Math.max(40, Math.round(parent.height * 0.075))
    primaryScreen: screen.primaryScreen
  }

  // Secondary outputs keep the wallpaper and clock but do not duplicate the
  // password prompt.
  Column {
    anchors.centerIn: parent
    spacing: 10
    visible: !screen.primaryScreen

    Text {
      anchors.horizontalCenter: parent.horizontalCenter
      text: "\u25CF"
      color: "#ccffffff"
      font.pixelSize: 46
    }

    Text {
      anchors.horizontalCenter: parent.horizontalCenter
      text: qsTr("屏幕已锁定")
      color: "#ccffffff"
      font.pixelSize: 18
      renderType: Text.NativeRendering
    }
  }

  // --- Login -----------------------------------------------------------------

  LoginCard {
    id: loginCard

    anchors.centerIn: parent
    anchors.verticalCenterOffset: vkbLoader.active
      ? -Math.round(Math.min(vkbLoader.height / 2, parent.height * 0.18)) : 0
    visible: screen.primaryScreen
    backend: screen.backend
    testMode: screen.testMode
    onRequestQuit: screen.requestQuit()
  }

  // --- Bottom controls -------------------------------------------------------

  Row {
    id: controls

    anchors.right: parent.right
    anchors.bottom: parent.bottom
    anchors.margins: 28
    spacing: 8
    visible: screen.primaryScreen && screen.ready

    LockButton {
      visible: screen.backend && screen.backend.kbLayouts
        && screen.backend.kbLayouts.length > 1
      text: screen.backend && screen.backend.currentKbLayout
        ? screen.backend.currentKbLayout.toUpperCase() : ""
      onClicked: screen.cycleLayout()
    }

    LockButton {
      visible: screen.backend && screen.backend.userSwitchAvailable
      text: qsTr("切换用户")
      onClicked: userSwitcher.open()
    }

    LockButton {
      visible: screen.virtualKeyboardAvailable
      text: "\u2328"
      tooltip: qsTr("虚拟键盘")
      onClicked: vkbLoader.active = !vkbLoader.active
    }

    LockButton {
      text: "\u23FB"
      tooltip: qsTr("电源")
      onClicked: powerMenu.open()
    }
  }

  UserSwitcher {
    id: userSwitcher
    backend: screen.backend
  }

  PowerMenu {
    id: powerMenu
    backend: screen.backend
    onTriggered: function(action) { screen.requestPower(action) }
  }

  Loader {
    id: vkbLoader

    anchors.horizontalCenter: parent.horizontalCenter
    anchors.bottom: parent.bottom
    width: Math.min(parent.width - 32, 900)
    height: item ? item.implicitHeight : 0
    active: false
    sourceComponent: Component {
      VirtualKeyboardPanel { }
    }

    onLoaded: {
      if (item) {
        item.width = Qt.binding(function() { return vkbLoader.width })
      }
    }
  }

}
