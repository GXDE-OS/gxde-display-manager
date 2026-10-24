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
import QtQuick.Effects

Item {
  id: frame

  property bool light: false
  property Item backdrop: null
  property int radius: 8
  property var trackedGeometry: []
  readonly property int backdropPadding: 32
  readonly property color windowColor: light ? "#f8f8f8" : "#252525"
  readonly property real windowAlpha: light ? 0.45 : 0.6

  readonly property rect backdropRect: {
    frame.trackedGeometry
    if (!frame.backdrop || width <= 0 || height <= 0) {
      return Qt.rect(0, 0, 0, 0)
    }
    return frame.mapToItem(frame.backdrop,
      -frame.backdropPadding, -frame.backdropPadding,
      width + frame.backdropPadding * 2, height + frame.backdropPadding * 2)
  }

  Item {
    id: shadowLayer

    anchors.fill: parent
    anchors.margins: -16
    layer.enabled: true
    layer.effect: MultiEffect {
      maskEnabled: true
      maskInverted: true
      maskSource: shadowCutout
      maskThresholdMin: 0.5
      maskSpreadAtMin: 1.0
    }

    Rectangle {
      id: shadowShape

      x: 16
      y: 16
      width: frame.width
      height: frame.height
      radius: frame.radius
      color: "black"
      visible: false
    }

    MultiEffect {
      anchors.fill: shadowShape
      source: shadowShape
      shadowEnabled: true
      shadowColor: "black"
      shadowOpacity: 0.2
      shadowBlur: 0.6
      blurMax: 16
      shadowVerticalOffset: 2
    }
  }

  Item {
    id: shadowCutout

    width: shadowLayer.width
    height: shadowLayer.height
    visible: false
    layer.enabled: true

    Rectangle {
      x: 16
      y: 16
      width: frame.width
      height: frame.height
      radius: frame.radius
      antialiasing: true
    }
  }

  Loader {
    active: frame.visible && frame.backdrop !== null
    sourceComponent: Item {
      ShaderEffectSource {
        id: backdropSource

        sourceItem: frame.backdrop
        sourceRect: frame.backdropRect
        visible: false
        live: true
      }

      Item {
        id: backdropMask

        width: frame.width + frame.backdropPadding * 2
        height: frame.height + frame.backdropPadding * 2
        visible: false
        layer.enabled: true

        Rectangle {
          x: frame.backdropPadding
          y: frame.backdropPadding
          width: frame.width
          height: frame.height
          radius: frame.radius
          antialiasing: true
        }
      }

      MultiEffect {
        x: -frame.backdropPadding
        y: -frame.backdropPadding
        width: backdropMask.width
        height: backdropMask.height
        source: backdropSource
        autoPaddingEnabled: false
        blurEnabled: true
        blur: 1.0
        blurMax: 32
        maskEnabled: true
        maskSource: backdropMask
        maskThresholdMin: 0.5
        maskSpreadAtMin: 1.0
      }
    }
  }

  Rectangle {
    anchors.fill: parent
    radius: frame.radius
    antialiasing: true
    color: Qt.rgba(frame.windowColor.r, frame.windowColor.g,
      frame.windowColor.b, frame.backdrop ? frame.windowAlpha : 1.0)
    border.width: 1
    border.color: frame.light ? "#1f000000" : "#80000000"

    Rectangle {
      anchors.fill: parent
      anchors.margins: 1
      radius: frame.radius - 1
      antialiasing: true
      color: "transparent"
      border.width: 1
      border.color: frame.light ? "#99ffffff" : "#1affffff"
    }
  }
}
