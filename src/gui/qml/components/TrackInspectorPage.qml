// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import Zrythm

ColumnLayout {
  id: root

  required property Track track
  required property UndoStack undoStack

  TrackOperator {
    id: trackOperator

    track: root.track
    undoStack: root.undoStack
  }

  ExpanderBox {
    Layout.fillWidth: true
    icon.source: ResourceManager.getIconUrl("gnome-icon-library", "general-properties-symbolic.svg")
    title: "Track Properties"

    frameContentItem: ColumnLayout {
      EditableLabel {
        id: trackNameLabel

        Layout.fillWidth: true
        text: root.track.name

        onAccepted: function (newText) {
          trackOperator.rename(newText);
        }
      }

      Label {
        text: root.track.comment
      }

      Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 40
        color: root.track.color
      }
    }
  }

  Loader {
    Layout.fillWidth: true
    active: (root.track.type === Track.Instrument || root.track.type === Track.Midi) && root.track.channel !== null && root.track.channel.midiFx !== null
    visible: active

    sourceComponent: PluginExpander {
      iconName: "audio-insert.svg"
      model: root.track.channel.midiFx
      title: "MIDI FX"
    }
  }

  Loader {
    Layout.fillWidth: true
    active: root.track.type === Track.Instrument
    visible: active

    sourceComponent: PluginExpander {
      iconName: "audio-insert.svg"
      model: root.track.channel.instruments
      title: qsTr("Instrument")
    }
  }

  Loader {
    Layout.fillWidth: true
    active: root.track.channel !== null && root.track.channel.inserts !== null
    visible: active

    sourceComponent: PluginExpander {
      iconName: "audio-insert.svg"
      model: root.track.channel.inserts
      title: qsTr("Inserts")
    }
  }

  Loader {
    Layout.fillWidth: true
    active: root.track.channel !== null
    visible: active

    sourceComponent: ExpanderBox {
      icon.source: ResourceManager.getIconUrl("zrythm-dark", "fader.svg")
      title: "Fader"

      frameContentItem: ColumnLayout {
        BalanceControl {
          Layout.fillWidth: true
          balanceParameter: root.track.channel.fader.balance
          undoStack: root.undoStack
        }

        RowLayout {
          Layout.fillWidth: true

          FaderButtons {
            fader: root.track.channel.fader
            track: root.track
          }

          FaderControl {
            faderGain: root.track.channel.fader.gain
            undoStack: root.undoStack
          }

          TrackMeters {
            id: meters

            Layout.fillHeight: true
            Layout.fillWidth: false
            channel: root.track.channel
          }

          Label {
            id: peakLabel

            readonly property real peak_in_dbfs: QmlUtils.amplitudeToDbfs(meters.currentPeak)

            text: {
              let txt_val = "Peak:\n";
              if (peak_in_dbfs < -98) {
                txt_val += "-∞";
              } else {
                let decimal_places = 1;
                if (peak_in_dbfs < -10) {
                  decimal_places = 0;
                }
                txt_val += Number(peak_in_dbfs).toFixed(decimal_places);
              }
              return txt_val + " db";
            }

            Binding {
              property: "color"
              target: peakLabel
              value: Style.dangerColor
              when: peakLabel.peak_in_dbfs > 0
            }
          }
        }
      }
    }
  }

  component PluginExpander: ExpanderBox {
    id: pluginExpander

    required property string iconName
    required property var model

    icon.source: ResourceManager.getIconUrl("zrythm-dark", iconName)

    frameContentItem: ListView {
      implicitHeight: contentHeight
      model: pluginExpander.model

      delegate: PluginSlotView {
        track: root.track
      }
    }
  }
}
