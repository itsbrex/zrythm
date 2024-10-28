import QtQuick
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

RowLayout {
    id: root

    default property alias content: root.children
    property int radius: Style.textFieldRadius
    property alias enableDropShadow: root.layer.enabled

    Layout.preferredWidth: implicitWidth
    Layout.maximumWidth: implicitWidth
    layer.enabled: false
    spacing: 0
    onChildrenChanged: {
        for (let i = 0; i < children.length; i++) {
            const child = children[i];
            child.Layout.fillWidth = true;
            child.layer.enabled = false;
            // Modify the existing background
            if (child.background && child.background instanceof Rectangle) {
                child.background.bottomLeftRadius = 0;
                child.background.topLeftRadius = 0;
                child.background.bottomRightRadius = 0;
                child.background.topRightRadius = 0;
                if (i === 0) {
                    child.background.bottomLeftRadius = root.radius;
                    child.background.topLeftRadius = root.radius;
                }
                if (i === children.length - 1) {
                    child.background.bottomRightRadius = root.radius;
                    child.background.topRightRadius = root.radius;
                }
            }
        }
    }

    layer.effect: DropShadowEffect {
    }

}