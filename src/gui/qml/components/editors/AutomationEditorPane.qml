// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

GridLayout {
  id: root

  required property AutomationEditor automationEditor
  required property ClipEditor clipEditor
  required property Project project
  required property AutomationRegion region

  columnSpacing: 0
  columns: 3
  rowSpacing: 0
  rows: 3

  ZrythmToolBar {
    id: topOfPianoRollToolbar

    leftItems: [
      ToolButton {
        checkable: true
        checked: false
        icon.source: ResourceManager.getIconUrl("font-awesome", "drum-solid.svg")

        ToolTip {
          text: qsTr("Drum Notation")
        }
      }
    ]
  }

  Ruler {
    id: ruler

    Layout.fillWidth: true
    editorSettings: root.automationEditor.editorSettings
    tempoMap: root.project.tempoMap
    transport: root.project.transport
  }

  ColumnLayout {
    Layout.alignment: Qt.AlignTop
    Layout.rowSpan: 3

    ToolButton {
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "chat-symbolic.svg")

      ToolTip {
        text: qsTr("Zoom In")
      }
    }
  }

  Item {
    id: automationLegend

    Layout.fillHeight: true
    Layout.preferredHeight: 480
  }

  UnifiedArrangerObjectsModel {
    id: unifiedObjectsModel

  }

  ItemSelectionModel {
    id: arrangerSelectionModel

    function getObjectFromUnifiedIndex(unifiedIndex: var): ArrangerObject {
      const sourceIndex = unifiedObjectsModel.mapToSource(unifiedIndex);
      return sourceIndex.data(ArrangerObjectListModel.ArrangerObjectPtrRole);
    }

    model: unifiedObjectsModel

    onSelectionChanged: (selected, deselected) => {
      console.log("Selection changed:", selectedIndexes.length, "items selected");
      if (selectedIndexes.length > 0) {
        const firstObject = selectedIndexes[0].data(ArrangerObjectListModel.ArrangerObjectPtrRole) as ArrangerObject;
        console.log("first selected object:", firstObject);
      }
    }
  }

  ArrangerObjectSelectionOperator {
    id: selectionOperator

    selectionModel: arrangerSelectionModel
    undoStack: root.project.undoStack
  }

  AutomationArranger {
    id: automationArranger

    Layout.fillHeight: true
    Layout.fillWidth: true
    arrangerContentHeight: automationLegend.height
    arrangerSelectionModel: arrangerSelectionModel
    automationEditor: root.automationEditor
    clipEditor: root.clipEditor
    objectCreator: root.project.arrangerObjectCreator
    ruler: ruler
    selectionOperator: selectionOperator
    snapGrid: root.project.snapGridEditor
    tempoMap: root.project.tempoMap
    tool: root.project.tool
    transport: root.project.transport
    undoStack: root.project.undoStack
    unifiedObjectsModel: unifiedObjectsModel
  }
}
