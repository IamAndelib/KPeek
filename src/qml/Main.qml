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
    // Fits small screens (e.g. 1366×768 at 125%) with room to spare.
    width: Math.min(Kirigami.Units.gridUnit * 48, Screen.desktopAvailableWidth * 0.85)
    height: Math.min(Kirigami.Units.gridUnit * 32, Screen.desktopAvailableHeight * 0.85)
    minimumWidth: Kirigami.Units.gridUnit * 20
    minimumHeight: Kirigami.Units.gridUnit * 12
    visible: false
    // No titlebar or border: a preview, not another window. The header bar moves it, the edges resize it,
    // and the compositor draws its shadow (PeekController::setWindow()).
    flags: Qt.Window | Qt.FramelessWindowHint

    // Closing the window closes the preview, which quits KPeek.
    onClosing: close => {
        close.accepted = false;
        PeekController.close();
    }

    header: QQC2.ToolBar {
        // Layouts don't mirror by themselves for right-to-left languages.
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true

        // The header is the titlebar: dragging anywhere on it except a button moves the window.
        // Buttons take their own presses, so this only ever sees the bar, the file name and the counter.
        DragHandler {
            target: null
            cursorShape: Qt.ClosedHandCursor
            onActiveChanged: if (active) root.startSystemMove()
        }

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
                textFormat: Text.PlainText
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
                    text: i18nc("@action:button", "Reset Zoom")
                    visible: Math.abs(input.zoom - 1) > 0.001
                    focusPolicy: Qt.NoFocus
                    onClicked: input.zoom = 1
                    QQC2.ToolTip.text: i18nc("@info:tooltip", "Reset zoom (Ctrl+0)")
                    QQC2.ToolTip.visible: hovered
                }

                QQC2.ToolButton {
                    icon.name: "zoom-out"
                    display: QQC2.AbstractButton.IconOnly
                    text: i18nc("@action:button", "Zoom Out")
                    enabled: input.zoom > input.minZoom
                    focusPolicy: Qt.NoFocus
                    onClicked: input.zoomBy(-1)
                    QQC2.ToolTip.text: i18nc("@info:tooltip", "Zoom out (Ctrl+−)")
                    QQC2.ToolTip.visible: hovered
                }
                QQC2.Label {
                    id: zoomLabel
                    text: i18nc("@info zoom level", "%1%", Math.round(input.zoom * 100))
                    horizontalAlignment: Text.AlignHCenter
                    font.features: { "tnum": 1 }
                    // Fixed width: "75%" and "400%" mustn't move the + button either.
                    Layout.preferredWidth: zoomLabelMetrics.advanceWidth + Kirigami.Units.smallSpacing * 2

                    TextMetrics {
                        id: zoomLabelMetrics
                        font: zoomLabel.font
                        text: i18nc("@info zoom level", "%1%", 8888) // the widest a zoom level gets
                    }
                }
                QQC2.ToolButton {
                    icon.name: "zoom-in"
                    display: QQC2.AbstractButton.IconOnly
                    text: i18nc("@action:button", "Zoom In")
                    enabled: input.zoom < input.maxZoom
                    focusPolicy: Qt.NoFocus
                    onClicked: input.zoomBy(1)
                    QQC2.ToolTip.text: i18nc("@info:tooltip", "Zoom in (Ctrl++)")
                    QQC2.ToolTip.visible: hovered
                }
            }

            QQC2.ToolButton {
                icon.name: "document-open"
                text: i18nc("@action:button", "Open")
                // Narrow windows: keep the file name readable.
                display: root.width < Kirigami.Units.gridUnit * 30 ? QQC2.AbstractButton.IconOnly : QQC2.AbstractButton.TextBesideIcon
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
            // Unload first: resetting the zoom while the old viewer exists would make it re-layout for nothing.
            viewerLoader.source = "";
            input.zoom = 1;
            viewerLoader.source = Qt.resolvedUrl("viewers/%1Viewer.qml".arg(PeekController.viewer));
        }
    }

    Loader {
        id: viewerLoader
        anchors.fill: parent
        focus: true
        // Unloaded while hidden so closing the preview stops video and audio playback.
        active: PeekController.showing
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true
        // A viewer whose QML modules are missing (e.g. a distribution without Qt Quick 3D or
        // Qt Multimedia) shows the info card rather than an empty window.
        onStatusChanged: {
            if (status === Loader.Error && !source.toString().endsWith("InfoViewer.qml")) {
                source = Qt.resolvedUrl("viewers/InfoViewer.qml");
            }
        }
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

    // Edges and corners that resize the frameless window. They sit on the window's root item (the header's parent),
    // above the header: anywhere lower in the stacking (the Controls overlay, contentItem.parent), the header
    // takes the pointer first and the top edge and corners never get it (seen on Wayland).
    Item {
        parent: root.header.parent
        anchors.fill: parent
        z: 1000000

        // The window's edge, visible on any background.
        Rectangle {
            anchors.fill: parent
            color: "transparent"
            border.color: Kirigami.ColorUtils.linearInterpolation(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, Kirigami.Theme.frameContrast)
        }

        Repeater {
            // [edges, x and y as fractions of the window, spans the full width, spans the full height]
            model: [
                [Qt.LeftEdge, 0, 0, 0, 1], [Qt.RightEdge, 1, 0, 0, 1], [Qt.TopEdge, 0, 0, 1, 0], [Qt.BottomEdge, 0, 1, 1, 0],
                [Qt.LeftEdge | Qt.TopEdge, 0, 0, 0, 0], [Qt.RightEdge | Qt.TopEdge, 1, 0, 0, 0],
                [Qt.LeftEdge | Qt.BottomEdge, 0, 1, 0, 0], [Qt.RightEdge | Qt.BottomEdge, 1, 1, 0, 0]
            ]

            Item {
                required property var modelData
                readonly property int edges: modelData[0]
                // Corners get a larger target than edges.
                readonly property int size: (modelData[3] || modelData[4] ? 1 : 2) * (Kirigami.Units.smallSpacing + 2)

                x: modelData[1] * (parent.width - size)
                y: modelData[2] * (parent.height - size)
                width: modelData[3] ? parent.width : size
                height: modelData[4] ? parent.height : size

                HoverHandler {
                    cursorShape: {
                        const e = parent.edges;
                        if (e === (Qt.LeftEdge | Qt.TopEdge) || e === (Qt.RightEdge | Qt.BottomEdge)) {
                            return Qt.SizeFDiagCursor;
                        }
                        if (e === (Qt.RightEdge | Qt.TopEdge) || e === (Qt.LeftEdge | Qt.BottomEdge)) {
                            return Qt.SizeBDiagCursor;
                        }
                        return e & (Qt.LeftEdge | Qt.RightEdge) ? Qt.SizeHorCursor : Qt.SizeVerCursor;
                    }
                }
                DragHandler {
                    target: null
                    onActiveChanged: if (active) root.startSystemResize(parent.edges)
                }
            }
        }
    }
}
