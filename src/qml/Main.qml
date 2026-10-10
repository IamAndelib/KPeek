// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import io.github.iamandelib.kpeek

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
                focusPolicy: Qt.NoFocus
                onClicked: PeekController.previous()
                QQC2.ToolTip.text: text
                QQC2.ToolTip.visible: hovered
            }
            QQC2.ToolButton {
                icon.name: "go-next"
                display: QQC2.AbstractButton.IconOnly
                text: i18nc("@action:button", "Next")
                enabled: PeekController.count > 1
                focusPolicy: Qt.NoFocus
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

            // Zoom controls, for every viewer where zooming means something. Reset sits on the left so
            // that when it appears, the title and position shift instead of the -/+ buttons (rapid
            // clicks on + must never land on reset).
            RowLayout {
                visible: input.zoomable
                spacing: 0

                QQC2.ToolButton {
                    icon.name: "edit-reset"
                    display: QQC2.AbstractButton.IconOnly
                    text: i18nc("@action:button", "Reset Zoom (Ctrl+0)")
                    visible: Math.abs(input.zoom - 1) > 0.001
                    focusPolicy: Qt.NoFocus
                    onClicked: input.zoom = 1
                    QQC2.ToolTip.text: text
                    QQC2.ToolTip.visible: hovered
                }

                QQC2.ToolButton {
                    icon.name: "zoom-out"
                    display: QQC2.AbstractButton.IconOnly
                    text: i18nc("@action:button", "Zoom Out (Ctrl+−)")
                    enabled: input.zoom > input.minZoom
                    focusPolicy: Qt.NoFocus
                    onClicked: input.zoomBy(-1)
                    QQC2.ToolTip.text: text
                    QQC2.ToolTip.visible: hovered
                }
                QQC2.Label {
                    text: i18nc("@info zoom level", "%1%", Math.round(input.zoom * 100))
                    horizontalAlignment: Text.AlignHCenter
                    font.features: { "tnum": 1 }
                    // Fixed width: "75%" and "400%" mustn't move the + button either.
                    Layout.preferredWidth: zoomLabelMetrics.advanceWidth + Kirigami.Units.smallSpacing * 2

                    TextMetrics {
                        id: zoomLabelMetrics
                        font: parent.font
                        text: i18nc("@info zoom level", "%1%", 400)
                    }
                }
                QQC2.ToolButton {
                    icon.name: "zoom-in"
                    display: QQC2.AbstractButton.IconOnly
                    text: i18nc("@action:button", "Zoom In (Ctrl++)")
                    enabled: input.zoom < input.maxZoom
                    focusPolicy: Qt.NoFocus
                    onClicked: input.zoomBy(1)
                    QQC2.ToolTip.text: text
                    QQC2.ToolTip.visible: hovered
                }
            }

            QQC2.ToolButton {
                icon.name: "document-open"
                text: i18nc("@action:button", "Open")
                focusPolicy: Qt.NoFocus
                onClicked: PeekController.openExternally()
                QQC2.ToolTip.text: i18nc("@info:tooltip", "Open with the default application (Enter)")
                QQC2.ToolTip.visible: hovered
            }
        }
    }

    // A fresh viewer for every file, so no state (scroll position, zoom, toggles) leaks into the next one.
    Connections {
        target: PeekController
        function onCurrentChanged() {
            input.zoom = 1;
            viewerLoader.source = "";
            viewerLoader.source = Qt.resolvedUrl("viewers/%1Viewer.qml".arg(PeekController.viewer));
        }
    }

    Loader {
        id: viewerLoader
        anchors.fill: parent
        focus: true
        // Unloaded while hidden so closing the preview stops video and audio playback.
        active: root.visible
        onLoaded: {
            if ("zoom" in item) {
                item.zoom = Qt.binding(() => input.zoom);
            }
        }
        Keys.onPressed: event => input.handleKey(event)
    }

    ViewerInput {
        id: input
        anchors.fill: viewerLoader
        viewer: viewerLoader.item
    }

    Shortcut {
        sequences: [StandardKey.ZoomIn, "Ctrl+="]
        onActivated: input.zoomBy(1)
    }
    Shortcut {
        sequences: [StandardKey.ZoomOut]
        onActivated: input.zoomBy(-1)
    }
    Shortcut {
        sequences: ["Ctrl+0"]
        onActivated: input.zoom = 1
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
