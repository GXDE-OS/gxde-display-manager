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
import QtQuick.Controls.Basic

Item {
  id: card

  property var backend: null
  property bool testMode: false
  property string errorText: ""
  property string statusText: ""

  signal requestQuit()

  implicitWidth: 380
  implicitHeight: content.implicitHeight + 48
  width: Math.max(260, Math.min(implicitWidth,
    parent ? parent.width - 64 : implicitWidth))
  height: implicitHeight
  enabled: backend !== null

  transform: Translate { id: shakeTranslate }

  function powerLabel(action) {
    switch (action) {
    case "shutdown": return qsTr("关机")
    case "reboot": return qsTr("重启")
    case "suspend": return qsTr("待机")
    case "hibernate": return qsTr("休眠")
    default: return action
    }
  }

  function focusPassword() {
    if (!visible || !enabled) return
    if (backend && backend.lockedOut) return
    passwordField.forceActiveFocus()
  }

  function submit() {
    if (!backend || backend.authenticating || backend.lockedOut) return
    if (passwordField.text.length === 0) {
      showError(qsTr("请输入密码"))
      return
    }
    errorText = ""
    statusText = ""
    backend.Authenticate(passwordField.text)
  }

  function showError(message) {
    errorText = String(message || qsTr("认证失败"))
    statusText = ""
    passwordField.selectAll()
    focusPassword()
    shake.restart()
  }

  function showSuccess() {
    errorText = ""
    statusText = qsTr("认证成功")
    passwordField.text = ""
  }

  function showPowerProgress(action) {
    errorText = ""
    statusText = qsTr("正在%1…").arg(powerLabel(action))
    passwordField.text = ""
  }

  function clearPassword() {
    passwordField.text = ""
    errorText = ""
    statusText = ""
  }

  onVisibleChanged: {
    if (visible) focusPassword()
  }

  Component.onCompleted: focusPassword()

  // Soft, translucent card. Keeping the background independent from the
  // wallpaper makes the password field readable on both light and dark images.
  Rectangle {
    anchors.fill: parent
    radius: 20
    color: "#b8121720"
    border.width: 1
    border.color: "#2effffff"

    Rectangle {
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.top: parent.top
      anchors.margins: 1
      height: 1
      color: "#26ffffff"
    }
  }

  Column {
    id: content

    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    anchors.margins: 24
    spacing: 0

    UserAvatar {
      anchors.horizontalCenter: parent.horizontalCenter
      width: 76
      height: 76
      radius: 18
      source: card.backend ? card.backend.currentUserAvatar : ""
      name: card.backend ? card.backend.currentUserDisplayName : ""
    }

    Text {
      width: parent.width
      topPadding: 12
      text: card.backend
        ? (card.backend.currentUserDisplayName || card.backend.currentUserName)
        : ""
      color: "#f5f8ff"
      font.pixelSize: 18
      font.weight: Font.Medium
      horizontalAlignment: Text.AlignHCenter
      elide: Text.ElideRight
      renderType: Text.NativeRendering
    }

    Text {
      width: parent.width
      topPadding: 2
      visible: card.testMode && card.statusText.length === 0
      text: qsTr("演示模式：任意密码解锁，Esc 退出")
      color: "#99c9d5e8"
      font.pixelSize: 11
      horizontalAlignment: Text.AlignHCenter
      renderType: Text.NativeRendering
    }

    Item {
      width: 1
      height: 16
    }

    // Power actions still require authentication; explain what the next
    // password submission will do and allow cancelling it.
    Rectangle {
      id: powerBanner

      width: parent.width
      height: visible ? 38 : 0
      visible: card.backend && card.backend.pendingPowerAction.length > 0
      radius: 9
      color: "#2b8a6a32"
      border.width: 1
      border.color: "#4dcaa25a"

      Text {
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        text: card.backend
          ? qsTr("输入密码以%1").arg(card.powerLabel(
              card.backend.pendingPowerAction)) : ""
        color: "#ffe6b8"
        font.pixelSize: 12
        renderType: Text.NativeRendering
      }

      LockButton {
        anchors.right: parent.right
        anchors.rightMargin: 4
        anchors.verticalCenter: parent.verticalCenter
        width: 48
        height: 30
        text: qsTr("取消")
        onClicked: {
          if (card.backend) card.backend.CancelPowerAction()
          card.focusPassword()
        }
      }
    }

    Item {
      width: 1
      height: powerBanner.visible ? 10 : 0
    }

    Item {
      width: parent.width
      height: 44

      TextField {
        id: passwordField

        anchors.fill: parent
        rightPadding: 48
        leftPadding: 14
        verticalAlignment: TextInput.AlignVCenter
        placeholderText: card.backend && card.backend.lockedOut
          ? qsTr("已锁定") : qsTr("密码")
        placeholderTextColor: "#80ffffff"
        color: "#f5f8ff"
        selectionColor: "#0081ff"
        selectedTextColor: "#ffffff"
        font.pixelSize: 15
        echoMode: TextInput.Password
        passwordCharacter: "\u25CF"
        inputMethodHints: Qt.ImhSensitiveData
          | Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
        enabled: card.backend && !card.backend.authenticating
          && !card.backend.lockedOut
        onAccepted: card.submit()
        onTextEdited: {
          if (card.errorText.length > 0) card.errorText = ""
          if (card.statusText.length > 0) card.statusText = ""
        }

        background: Rectangle {
          radius: 10
          color: passwordField.activeFocus ? "#26ffffff" : "#1affffff"
          border.width: 1
          border.color: card.errorText.length > 0 ? "#ff9a8a"
            : passwordField.activeFocus ? "#66ffffff" : "#1fffffff"

          Behavior on border.color {
            ColorAnimation { duration: 140 }
          }
        }
      }

      LockButton {
        anchors.right: parent.right
        anchors.rightMargin: 4
        anchors.verticalCenter: parent.verticalCenter
        width: 36
        height: 36
        text: "\u2192"
        enabled: card.backend && !card.backend.authenticating
          && !card.backend.lockedOut
          && passwordField.text.length > 0
        onClicked: card.submit()
      }
    }

    Item {
      width: 1
      height: 10
    }

    Row {
      width: parent.width
      height: Math.max(busyIndicator.visible ? 22 : 0,
                       message.implicitHeight)
      spacing: 8

      BusyIndicator {
        id: busyIndicator

        anchors.verticalCenter: parent.verticalCenter
        width: 22
        height: 22
        running: card.backend && card.backend.authenticating
        visible: running
      }

      Text {
        id: message

        anchors.verticalCenter: parent.verticalCenter
        width: parent.width - (busyIndicator.visible ? 30 : 0)
        text: {
          if (card.errorText.length > 0) return card.errorText
          if (card.backend && card.backend.lockedOut) {
            return qsTr("尝试次数过多，请在 %1 分钟后重试")
              .arg(card.backend.lockMinutes)
          }
          return card.statusText
        }
        color: card.errorText.length > 0 || (card.backend && card.backend.lockedOut)
          ? "#ff9a8a" : "#b8d6c0"
        font.pixelSize: 12
        wrapMode: Text.Wrap
        renderType: Text.NativeRendering
      }
    }

    Item {
      width: 1
      height: 8
    }

    Text {
      width: parent.width
      text: qsTr("按 Enter 解锁")
      color: "#66c9d5e8"
      font.pixelSize: 11
      horizontalAlignment: Text.AlignHCenter
      renderType: Text.NativeRendering
    }
  }

  SequentialAnimation {
    id: shake

    NumberAnimation {
      target: shakeTranslate
      property: "x"
      to: -9
      duration: 50
      easing.type: Easing.OutQuad
    }
    NumberAnimation {
      target: shakeTranslate
      property: "x"
      to: 8
      duration: 80
      easing.type: Easing.InOutQuad
    }
    NumberAnimation {
      target: shakeTranslate
      property: "x"
      to: -5
      duration: 70
      easing.type: Easing.InOutQuad
    }
    NumberAnimation {
      target: shakeTranslate
      property: "x"
      to: 2
      duration: 60
      easing.type: Easing.InOutQuad
    }
    NumberAnimation {
      target: shakeTranslate
      property: "x"
      to: 0
      duration: 50
      easing.type: Easing.OutQuad
    }
  }

}
