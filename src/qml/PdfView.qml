// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibsriz@proton.me>

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kpeek

// Scrollable page list for a PDF; shared by the PDF viewer and converted office documents.
Rectangle {
    id: root

    property alias url: document.url
    property real zoom: 1.0
    readonly property real fitWidth: Math.max(Kirigami.Units.gridUnit * 10, view.width - Kirigami.Units.largeSpacing * 2)
    readonly property real pageWidth: fitWidth * zoom

    color: Kirigami.Theme.alternateBackgroundColor
    focus: true
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Plus || event.key === Qt.Key_Equal) {
            root.zoom = Math.min(4, root.zoom * 1.25);
            event.accepted = true;
        } else if (event.key === Qt.Key_Minus) {
            root.zoom = Math.max(0.25, root.zoom / 1.25);
            event.accepted = true;
        } else {
            ScrollKeys.handle(event, view);
        }
    }

    PdfDocument {
        id: document
    }

    ListView {
        id: view
        anchors.fill: parent
        visible: document.pageCount > 0
        clip: true
        spacing: Kirigami.Units.largeSpacing
        topMargin: Kirigami.Units.largeSpacing
        bottomMargin: Kirigami.Units.gridUnit * 3
        contentWidth: Math.max(width, root.pageWidth + Kirigami.Units.largeSpacing * 2)
        flickableDirection: Flickable.AutoFlickIfNeeded
        boundsBehavior: Flickable.StopAtBounds
        cacheBuffer: height
        model: document.pageCount

        QQC2.ScrollBar.vertical: QQC2.ScrollBar {}
        QQC2.ScrollBar.horizontal: QQC2.ScrollBar {}

        delegate: Item {
            required property int index
            readonly property size pointSize: document.pageSize(index)

            width: view.contentWidth
            height: root.pageWidth * pointSize.height / Math.max(1, pointSize.width)

            Rectangle {
                id: page
                anchors.horizontalCenter: parent.horizontalCenter
                width: root.pageWidth
                height: parent.height
                color: "white"
                border.color: Kirigami.ColorUtils.linearInterpolation(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.2)

                Image {
                    anchors.fill: parent
                    source: document.pageSource(index)
                    sourceSize.width: Math.round(page.width * Screen.devicePixelRatio)
                    asynchronous: true
                    cache: false
                    retainWhileLoading: true
                }
            }
        }
    }

    // Page position and zoom controls.
    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Kirigami.Units.largeSpacing
        visible: document.pageCount > 0
        width: controls.implicitWidth + Kirigami.Units.smallSpacing * 2
        height: controls.implicitHeight + Kirigami.Units.smallSpacing * 2
        radius: Kirigami.Units.cornerRadius
        color: Qt.alpha(Kirigami.Theme.backgroundColor, 0.92)
        border.color: Kirigami.ColorUtils.linearInterpolation(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.2)

        RowLayout {
            id: controls
            anchors.centerIn: parent
            spacing: Kirigami.Units.smallSpacing

            QQC2.Label {
                readonly property int current: Math.max(0, view.indexAt(view.width / 2, view.contentY + view.height / 2))
                text: i18nc("@info page position", "Page %1 of %2", current + 1, document.pageCount)
                Layout.leftMargin: Kirigami.Units.smallSpacing
            }
            QQC2.ToolButton {
                icon.name: "zoom-out"
                display: QQC2.AbstractButton.IconOnly
                text: i18nc("@action:button", "Zoom Out")
                onClicked: root.zoom = Math.max(0.25, root.zoom / 1.25)
            }
            QQC2.ToolButton {
                icon.name: "zoom-fit-width"
                display: QQC2.AbstractButton.IconOnly
                text: i18nc("@action:button", "Fit Width")
                onClicked: root.zoom = 1.0
            }
            QQC2.ToolButton {
                icon.name: "zoom-in"
                display: QQC2.AbstractButton.IconOnly
                text: i18nc("@action:button", "Zoom In")
                onClicked: root.zoom = Math.min(4, root.zoom * 1.25)
            }
        }
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: document.failed || document.locked
        icon.name: document.locked ? "object-locked" : "dialog-error"
        text: document.locked ? i18nc("@info", "This document is password-protected") : i18nc("@info", "This document could not be opened")
    }
}
