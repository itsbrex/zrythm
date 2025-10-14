// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import Zrythm

Control {
  id: root

  required property ClipPlaybackService clipPlaybackService
  required property ClipSlot clipSlot
  required property ArrangerObjectCreator objectCreator
  readonly property ArrangerObject region: clipSlot.region
  required property Scene scene
  required property Track track

  signal filesDropped(var filePaths)
  signal slotLaunched

  hoverEnabled: true

  Rectangle {
    anchors.fill: parent
    clip: true
    color: {
      let c = root.clipSlot.region ? root.track.color : palette.button;
      return Style.adjustColorForHoverOrVisualFocusOrDown(c, root.hovered, root.activeFocus, tapHandler.pressed);
    }
    radius: Style.textFieldRadius

    Loader {
      active: root.clipSlot.region !== null && root.clipSlot.region.type === ArrangerObject.MidiRegion
      anchors.fill: parent
      visible: active

      sourceComponent: MidiRegionContent {
        contentHeight: parent.height
        contentWidth: parent.width
        region: root.clipSlot.region as MidiRegion
      }
    }

    Loader {
      active: root.clipSlot.region !== null && root.clipSlot.region.type === ArrangerObject.AudioRegion
      anchors.fill: parent
      visible: active

      sourceComponent: AudioRegionContent {
        contentHeight: parent.height
        contentWidth: parent.width
        region: root.clipSlot.region as AudioRegion
      }
    }

    ClipLaunchButton {
      id: playButton

      clipPlaybackService: root.clipPlaybackService
      clipSlot: root.clipSlot
      enabled: root.region !== null
      track: root.track

      anchors {
        left: parent.left
        top: parent.top
      }
    }

    // Playback position indicator
    Rectangle {
      id: playbackPositionIndicator

      color: palette.text
      opacity: 0.6
      visible: root.clipSlot.state === ClipSlot.Playing || root.clipSlot.state === ClipSlot.PlayQueued
      width: 2

      anchors {
        bottom: parent.bottom
        top: parent.top
      }

      // Update position based on playback progress
      Timer {
        interval: 16 // ~60fps
        repeat: true
        running: playbackPositionIndicator.visible

        onTriggered: {
          let position = root.clipPlaybackService.getClipPlaybackPosition(root.clipSlot);
          if (position >= 0) {
            playbackPositionIndicator.x = position * root.width;
          }
        }
      }
    }

    TapHandler {
      id: tapHandler

      onDoubleTapped: {
        console.log("double tapped");
        let midiRegion = root.objectCreator.addEmptyMidiRegionToClip(root.track, root.clipSlot) as MidiRegion;
        root.objectCreator.addMidiNote(midiRegion, 0, 64);
        root.objectCreator.addMidiNote(midiRegion, 120, 68);
      }
    }

    DropArea {
      id: dropArea

      anchors.fill: parent

      onDropped: drop => {
        console.log("Drop on clip slot");
        let uniqueFilePaths = DragUtils.getUniqueFilePaths(drop);
        root.filesDropped(uniqueFilePaths);
      }
    }
  }
}
