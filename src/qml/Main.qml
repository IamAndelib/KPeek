// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibsriz@proton.me>

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kpeek

QQC2.ApplicationWindow {
    id: root

    title: PeekController.fileName
    width: Kirigami.Units.gridUnit * 48
    height: Kirigami.Units.gridUnit * 32
    minimumWidth: Kirigami.Units.gridUnit * 20
    minimumHeight: Kirigami.Units.gridUnit * 12
    visible: false

    // Closing the window only hides it; the process stays resident for the next preview.
    onClosing: close => {
        close.accepted = false;
        PeekController.close();
    }

    header: QQC2.ToolBar {
        RowLayout {
            anchors.fill: parent
            spacing: Kirigami.Units.smallSpacing

            QQC2.ToolButton {
                icon.name: "go-previous"
                display: QQC2.AbstractButton.IconOnly
                text: i18nc("@action:button", "Previous")
                enabled: PeekController.count > 1
                onClicked: PeekController.previous()
                QQC2.ToolTip.text: text
                QQC2.ToolTip.visible: hovered
            }
            QQC2.ToolButton {
                icon.name: "go-next"
                display: QQC2.AbstractButton.IconOnly
                text: i18nc("@action:button", "Next")
                enabled: PeekController.count > 1
                onClicked: PeekController.next()
                QQC2.ToolTip.text: text
                QQC2.ToolTip.visible: hovered
            }

            Kirigami.Icon {
                source: PeekController.iconName
                fallback: "unknown"
                Layout.preferredWidth: Kirigami.Units.iconSizes.small
                Layout.preferredHeight: Kirigami.Units.iconSizes.small
                Layout.leftMargin: Kirigami.Units.smallSpacing
            }
            QQC2.Label {
                text: PeekController.fileName
                elide: Text.ElideMiddle
                font.bold: true
                Layout.fillWidth: true
            }
            QQC2.Label {
                visible: PeekController.count > 1
                text: i18nc("@info position in file list", "%1 / %2", PeekController.index + 1, PeekController.count)
                opacity: 0.7
            }

            QQC2.ToolButton {
                icon.name: "document-open"
                text: i18nc("@action:button", "Open")
                onClicked: PeekController.openExternally()
                QQC2.ToolTip.text: i18nc("@info:tooltip", "Open with the default application (Enter)")
                QQC2.ToolTip.visible: hovered
            }
        }
    }

    Loader {
        id: viewerLoader
        anchors.fill: parent
        focus: true
        // Unloaded while hidden so closing the preview stops video and audio playback.
        active: root.visible
        source: PeekController.viewer.length > 0 ? Qt.resolvedUrl("viewers/%1Viewer.qml".arg(PeekController.viewer)) : ""
    }

    Shortcut {
        // Meta+Space is the default Dolphin shortcut that opens a preview, so it closes it here too.
        sequences: [StandardKey.Cancel, "Space", "Meta+Space", "Q"]
        onActivated: PeekController.close()
    }
    Shortcut {
        // Up/Down and Page keys scroll inside documents, so only Left/Right switch files.
        sequences: ["Right"]
        onActivated: PeekController.next()
    }
    Shortcut {
        sequences: ["Left"]
        onActivated: PeekController.previous()
    }
    Shortcut {
        sequences: ["Return", "Enter"]
        onActivated: PeekController.openExternally()
    }
}
