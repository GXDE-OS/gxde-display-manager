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
import QtQuick.Effects
import QtQuick.Window

Window {
  id: root

  property url backgroundUri: ""
  property date now: new Date()
  property color iconTint: "white"
  property bool isGxdeOs: false
  property string gxdeReleaseLabel: ""
  property var userModel: []
  property var sessionModel: []
  property int selectedSessionIndex: sessionModel.length > 0 ? 0 : -1
  readonly property var selectedSession: selectedSessionIndex >= 0
    && selectedSessionIndex < sessionModel.length
      ? sessionModel[selectedSessionIndex] : null
  property real activeBackgroundScale: 1.08
  property real activeBackgroundBlur: 0.55
  property real loginProgress: clockView.positionedAtTop ? 1.0 : 0.0
  property real dialogProgress: passwordCard.shown ? 1.0 : 0.0
  property bool authBackendAvailable: false
  property QtObject powerBackend: null
  readonly property var availablePowerActions: powerBackend
    ? powerBackend.availableActions : []
  property bool virtualKeyboardRequested: false
  readonly property real keyboardInset: virtualKeyboard.visible
    ? (virtualKeyboard.height + virtualKeyboard.screenMargin)
      * virtualKeyboard.slide
    : 0
  readonly property var weekdays: [
    "星期日",
    "星期一",
    "星期二",
    "星期三",
    "星期四",
    "星期五",
    "星期六"
  ]

  signal loginRequested()
  signal sessionSelected(var session)
  signal userSelected(int index, var user)
  signal passwordSubmitted(int index, var user, string password)
  signal manualLoginRequested()
  signal powerRequested(string action)
  onPowerRequested: (action) => {
    if (powerBackend) {
      powerBackend.Request(action)
    }
  }

  Connections {
    target: root.powerBackend
    function onRequestFailed(message) {
      powerError.text = message
      powerErrorPopup.open()
    }
  }

  Popup {
    id: powerErrorPopup
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(420, root.width - 40)
    padding: 20
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    background: DtkFrame { backdrop: windowContent }
    contentItem: Column {
      spacing: 16
      Label {
        id: powerError
        width: parent.width
        wrapMode: Text.Wrap
        color: "white"
      }
      Button {
        text: qsTr("关闭")
        anchors.right: parent.right
        onClicked: powerErrorPopup.close()
      }
    }
  }

  width: 640
  height: 480
  visible: true
  title: "GXDM Greeter"
  color: "#101010"

  Behavior on dialogProgress {
    NumberAnimation {
      duration: 180
      easing.type: Easing.OutCubic
    }
  }

  Behavior on loginProgress {
    NumberAnimation {
      duration: 250
      easing.type: Easing.Linear
    }
  }

  function dateLabel(value) {
    return value.getFullYear() + "年"
      + (value.getMonth() + 1) + "月"
      + value.getDate() + "日 "
      + weekdays[value.getDay()]
  }

  function openPasswordDialog(index, user) {
    passwordCard.askUserName = false
    passwordCard.userIndex = index
    passwordCard.user = user
    passwordCard.userName = userSelecter.fullNameFor(user)
    passwordCard.userAvatar = userSelecter.avatarFor(user)
    passwordCard.open()
  }

  function openManualLoginDialog() {
    passwordCard.askUserName = true
    passwordCard.userIndex = -1
    passwordCard.user = null
    passwordCard.userName = ""
    passwordCard.userAvatar = ""
    passwordCard.open()
  }

  function showPasswordError(message) {
    passwordCard.showError(message || qsTr("鉴权失败"))
    authFailureGlow.flash()
  }

  function requestLogin() {
    clockView.positionedAtTop = true
    root.loginRequested()
  }

  Timer {
    interval: 1000
    running: true
    repeat: true
    onTriggered: root.now = new Date()
  }

  Item {
    id: windowContent

    anchors.fill: parent

    Item {
      id: backgroundLayer

      anchors.fill: parent
      scale: 1.0 + (root.activeBackgroundScale - 1.0) * root.loginProgress

      Item {
        id: backgroundSource

        anchors.fill: parent

        Image {
          anchors.fill: parent
          source: root.backgroundUri
          fillMode: Image.PreserveAspectCrop
          asynchronous: true
          cache: false
          smooth: true
          visible: root.backgroundUri.toString() !== ""
        }

        Rectangle {
          anchors.fill: parent
          visible: root.backgroundUri.toString() === ""
          gradient: Gradient {
            GradientStop { position: 0; color: "#e9def8" }
            GradientStop { position: 1; color: "#cfdff2" }
          }
        }
      }

      MultiEffect {
        anchors.fill: backgroundSource
        source: backgroundSource
        visible: root.loginProgress > 0
        blurEnabled: true
        blurMax: 32
        blur: root.activeBackgroundBlur * root.loginProgress
        autoPaddingEnabled: false
      }
    }

    Rectangle {
      anchors.fill: parent
      color: "transparent"
      gradient: Gradient {
        GradientStop { position: 0.0; color: "#00000000" }
        GradientStop { position: 0.55; color: "#00000000" }
        GradientStop { position: 1.0; color: "#36000000" }
      }
    }

    Item {
      id: sceneContent

      anchors.fill: parent
      opacity: 1.0 - 0.7 * root.dialogProgress
      layer.enabled: root.dialogProgress > 0
      layer.effect: MultiEffect {
        blurEnabled: true
        blurMax: 32
        blur: 0.8 * root.dialogProgress
        autoPaddingEnabled: false
      }

      Item {
        id: clockView

        property bool positionedAtTop: false

        x: (parent.width - width) / 2
        y: positionedAtTop ? 72 : (parent.height - height) / 2
        width: Math.max(timeLabel.width, promptLabel.width)
        height: timeLabel.inkHeight + 8 + promptLabel.inkHeight

        Behavior on y {
          NumberAnimation {
            duration: 250
            easing.type: Easing.OutExpo
          }
        }

        ShadowText {
          id: timeLabel

          anchors.horizontalCenter: parent.horizontalCenter
          y: -inkTop
          text: Qt.formatTime(root.now, "hh:mm")
          pixelSize: 48
          weight: Font.Black
          shadowOpacity: 0.24
        }

        ShadowText {
          id: promptLabel

          anchors.horizontalCenter: parent.horizontalCenter
          y: timeLabel.inkHeight + 8 - inkTop
          text: dateLabel(root.now)
          pixelSize: 16
          weight: Font.Light
          shadowOpacity: 0.22
        }
      }

      UserSelecter {
        id: userSelecter

        anchors.centerIn: parent
        anchors.verticalCenterOffset: -Math.min(root.keyboardInset / 2,
          Math.max(0, (parent.height - height) / 2
            - (clockView.y + clockView.height + 24)))
        width: Math.min(implicitWidth, Math.max(0, parent.width - 48))
        model: root.userModel
        foregroundColor: "white"
        visible: clockView.positionedAtTop || root.loginProgress > 0
        enabled: clockView.positionedAtTop
        opacity: root.loginProgress
        revealed: clockView.positionedAtTop
        onUserConfirmed: (index, user) => {
          root.userSelected(index, user)
          root.openPasswordDialog(index, user)
        }
        onNotListedRequested: {
          root.manualLoginRequested()
          root.openManualLoginDialog()
        }
      }
    }

    Rectangle {
      id: dialogScrim

      anchors.fill: parent
      color: "#33000000"
      opacity: root.dialogProgress
      visible: opacity > 0

      MouseArea {
        anchors.fill: parent
        enabled: passwordCard.shown
        hoverEnabled: true
        acceptedButtons: Qt.AllButtons
        onPressed: passwordCard.close()
        onWheel: (wheel) => wheel.accepted = true
      }
    }

    Item {
      id: gxdeInfoView

      property bool meetsAdditionalVisibilityConditions: true
      property real revealProgress: clockView.positionedAtTop ? 1.0 : 0.0
      readonly property real bottomMargin: 12
      readonly property real hiddenY: parent.height - height / 2
      readonly property real shownY: parent.height - height - bottomMargin
      readonly property real logoRightInkPadding: gxdeLogo.width * 5 / 150
      readonly property real versionLeftMargin: 5 - logoRightInkPadding
      readonly property rect versionInkBounds:
        versionMetrics.tightBoundingRect(gxdeVersion.text)
      readonly property real versionInkTop:
        gxdeVersion.baselineOffset + versionInkBounds.y

      anchors.left: parent.left
      anchors.leftMargin: 12
      visible: root.isGxdeOs
        && root.gxdeReleaseLabel.length > 0
        && (clockView.positionedAtTop || revealProgress > 0)
        && meetsAdditionalVisibilityConditions
      opacity: revealProgress
      y: hiddenY + (shownY - hiddenY) * revealProgress - root.keyboardInset
      width: gxdeLogo.width + versionLeftMargin + gxdeVersion.implicitWidth
      height: gxdeLogo.height

      Behavior on revealProgress {
        NumberAnimation {
          duration: 140
          easing.type: Easing.OutCubic
        }
      }

      FontMetrics {
        id: versionMetrics
        font: gxdeVersion.font
      }

      Image {
        id: gxdeLogo
        anchors.bottom: parent.bottom
        anchors.left: parent.left

        width: Math.round(height * 150 / 35)
        height: 32
        source: "qrc:/icons/gxde.png"
        fillMode: Image.PreserveAspectFit
        smooth: true
        mipmap: false
      }

      Text {
        id: gxdeVersion
        anchors.left: gxdeLogo.right
        anchors.leftMargin: gxdeInfoView.versionLeftMargin
        y: -gxdeInfoView.versionInkTop
        text: root.gxdeReleaseLabel
        color: "#d0d0d0"
        font.family: "Source Han Sans"
        font.pixelSize: 10
        font.weight: Font.Light
        renderType: Text.NativeRendering
      }
    }

    Row {
      id: operationView

      property real revealProgress: clockView.positionedAtTop ? 1.0 : 0.0
      readonly property real bottomMargin: 12
      readonly property real hiddenY: parent.height - height / 2
      readonly property real shownY: parent.height - height - bottomMargin

      anchors.right: parent.right
      anchors.rightMargin: 12
      visible: clockView.positionedAtTop || revealProgress > 0
      opacity: revealProgress
      y: hiddenY + (shownY - hiddenY) * revealProgress - root.keyboardInset
      spacing: 12

      Behavior on revealProgress {
        NumberAnimation {
          duration: 140
          easing.type: Easing.OutCubic
        }
      }

      IconButton {
        id: sessionButton

        objectName: "sessionButton"
        source: sessionMenu.iconFor(root.selectedSession)
        tint: "transparent"
        visible: root.sessionModel.length > 0
        Accessible.name: qsTr("选择桌面会话：")
          + (root.selectedSession ? root.selectedSession.name : "")
        onClicked: {
          powerMenu.close()
          sessionMenu.opened ? sessionMenu.close() : sessionMenu.open()
        }
      }

      IconButton {
        source: "qrc:/icons/keyboard.svg"
        tint: root.iconTint
        visible: keyboardLoader.status === Loader.Ready
        onClicked: root.virtualKeyboardRequested
          = !root.virtualKeyboardRequested
      }

      IconButton {
        id: powerButton

        source: "qrc:/icons/power.svg"
        tint: root.iconTint
        onClicked: {
          sessionMenu.close()
          powerMenu.opened ? powerMenu.close() : powerMenu.open()
        }
      }
    }
  }

  SessionMenu {
    id: sessionMenu

    readonly property point anchor: {
      operationView.x
      operationView.y
      return sessionButton.mapToItem(parent, sessionButton.width, 0)
    }

    objectName: "sessionMenu"
    parent: Overlay.overlay
    z: 20
    width: Math.min(implicitWidth, root.width - 24)
    maximumHeight: Math.max(46, Math.min(320, anchor.y - 20))
    x: Math.max(12, Math.round(anchor.x - width))
    y: Math.round(anchor.y - height - 8)
    backdrop: windowContent
    sessions: root.sessionModel
    selectedIndex: root.selectedSessionIndex
    onSelected: (index) => {
      root.selectedSessionIndex = index
      root.sessionSelected(root.selectedSession)
    }
    onClosed: {
      if (passwordCard.opened) {
        passwordCard.forceActiveFocus()
      } else {
        keyCatcher.forceActiveFocus()
      }
    }
  }

  PowerMenu {
    id: powerMenu

    readonly property point anchor: {
      operationView.x
      operationView.y
      return powerButton.mapToItem(parent, powerButton.width, 0)
    }

    parent: Overlay.overlay
    z: 20
    x: Math.round(anchor.x - width)
    y: Math.round(anchor.y - height - 8)
    backdrop: windowContent
    availableActions: root.availablePowerActions
    originCorner: Item.BottomRight
    onTriggered: (action) => root.powerRequested(action)
    onClosed: {
      if (passwordCard.opened) {
        passwordCard.forceActiveFocus()
      } else {
        keyCatcher.forceActiveFocus()
      }
    }
  }

  Item {
    id: keyCatcher

    anchors.fill: parent
    focus: true

    Component.onCompleted: forceActiveFocus()

    Keys.onPressed: (event) => {
      if (event.key === Qt.Key_Escape) {
        clockView.positionedAtTop = false
        root.virtualKeyboardRequested = false
      } else {
        root.requestLogin()
      }
      event.accepted = true
    }
  }

  TapHandler {
    acceptedButtons: Qt.AllButtons
    onTapped: root.requestLogin()
  }

  PasswordCard {
    id: passwordCard

    property int userIndex: -1
    property var user: null

    parent: Overlay.overlay
    x: Math.round((parent.width - width) / 2 + shakeOffset)
    y: Math.round((parent.height - root.keyboardInset - height) / 2)
    backdrop: windowContent
    onSubmitted: (userName, password) => {
      root.passwordSubmitted(userIndex, askUserName
        ? { "username": userName, "fullName": userName } : user, password)
      if (!root.authBackendAvailable) {
        busy = true
        fakeAuthTimer.restart()
      }
    }
    onClosed: {
      fakeAuthTimer.stop()
      keyCatcher.forceActiveFocus()
    }
  }

  Popup {
    id: virtualKeyboard

    property real slide: 0

    readonly property real screenMargin: 12

    parent: Overlay.overlay
    z: 10
    width: Math.min(parent.width - 24, 960)
    x: Math.round((parent.width - width) / 2)
    y: parent.height - (height + screenMargin) * slide
    padding: 8
    modal: false
    dim: false
    focus: false
    closePolicy: Popup.NoAutoClose
    visible: root.virtualKeyboardRequested && clockView.positionedAtTop
      && keyboardLoader.status === Loader.Ready

    background: DtkFrame {
      backdrop: windowContent
      trackedGeometry: [virtualKeyboard.x, virtualKeyboard.y,
        virtualKeyboard.visible]
    }

    enter: Transition {
      NumberAnimation {
        property: "slide"
        from: 0.0
        to: 1.0
        duration: 220
        easing.type: Easing.OutCubic
      }
    }

    exit: Transition {
      NumberAnimation {
        property: "slide"
        to: 0.0
        duration: 180
        easing.type: Easing.InCubic
      }
    }

    contentItem: Item {
      implicitHeight: keyboardLoader.item ? keyboardLoader.item.height : 0

      Loader {
        id: keyboardLoader

        width: parent.width
        asynchronous: true
        source: "VirtualKeyboardPanel.qml"
      }
    }
  }

  Timer {
    id: fakeAuthTimer

    interval: 400
    onTriggered: root.showPasswordError("")
  }

  Item {
    id: authFailureGlow

    readonly property color glowColor: "#be543c"
    readonly property real glowSize: Math.min(width, height) * 0.14

    function flash() {
      flashAnimation.restart()
    }

    parent: Overlay.overlay
    anchors.fill: parent
    z: 100
    opacity: 0
    visible: opacity > 0

    SequentialAnimation {
      id: flashAnimation

      NumberAnimation {
        target: authFailureGlow
        property: "opacity"
        to: 1.0
        duration: 160
        easing.type: Easing.OutCubic
      }
      PauseAnimation {
        duration: 700
      }
      NumberAnimation {
        target: authFailureGlow
        property: "opacity"
        to: 0.0
        duration: 900
        easing.type: Easing.InOutQuad
      }
    }

    component EdgeGlow: Rectangle {
      required property real edgeRotation

      color: "transparent"
      rotation: edgeRotation
      gradient: Gradient {
        GradientStop {
          position: 0.0
          color: Qt.rgba(authFailureGlow.glowColor.r,
            authFailureGlow.glowColor.g, authFailureGlow.glowColor.b, 0.6)
        }
        GradientStop {
          position: 0.35
          color: Qt.rgba(authFailureGlow.glowColor.r,
            authFailureGlow.glowColor.g, authFailureGlow.glowColor.b, 0.22)
        }
        GradientStop {
          position: 1.0
          color: Qt.rgba(authFailureGlow.glowColor.r,
            authFailureGlow.glowColor.g, authFailureGlow.glowColor.b, 0.0)
        }
      }
    }

    EdgeGlow {
      edgeRotation: 0
      x: 0
      y: 0
      width: parent.width
      height: parent.glowSize
    }

    EdgeGlow {
      edgeRotation: 180
      x: 0
      y: parent.height - height
      width: parent.width
      height: parent.glowSize
    }

    EdgeGlow {
      edgeRotation: -90
      x: (parent.glowSize - parent.height) / 2
      y: (parent.height - parent.glowSize) / 2
      width: parent.height
      height: parent.glowSize
    }

    EdgeGlow {
      edgeRotation: 90
      x: parent.width - (parent.glowSize + parent.height) / 2
      y: (parent.height - parent.glowSize) / 2
      width: parent.height
      height: parent.glowSize
    }
  }

  component ShadowText: Item {
    id: shadowText

    property alias text: label.text
    property int pixelSize: 24
    property int weight: Font.Normal
    property real shadowOpacity: 0.25
    readonly property rect inkBounds: textMetrics.tightBoundingRect(label.text)
    readonly property real inkTop: label.baselineOffset + inkBounds.y
    readonly property real inkHeight: inkBounds.height

    implicitWidth: label.implicitWidth + 4
    implicitHeight: label.implicitHeight + 4
    width: implicitWidth
    height: implicitHeight

    FontMetrics {
      id: textMetrics
      font: label.font
    }

    Text {
      x: 2
      y: 2
      text: label.text
      color: Qt.rgba(0, 0, 0, shadowText.shadowOpacity)
      font.family: "Source Han Sans"
      font.pixelSize: shadowText.pixelSize
      font.weight: shadowText.weight
      renderType: Text.NativeRendering
    }

    Text {
      id: label
      color: "white"
      font.family: "Source Han Sans"
      font.pixelSize: shadowText.pixelSize
      font.weight: shadowText.weight
      renderType: Text.NativeRendering
    }
  }

  component IconButton: ToolButton {
    id: iconButton

    property url source
    property color tint: "white"

    width: 32
    height: 32
    padding: 4
    focusPolicy: Qt.NoFocus
    hoverEnabled: true
    display: AbstractButton.IconOnly
    icon.source: iconButton.source
    icon.width: 24
    icon.height: 24
    icon.color: iconButton.tint
    opacity: hovered ? 1.0 : 0.86

    background: Item {
    }

    HoverHandler {
      cursorShape: Qt.PointingHandCursor
    }
  }
}
