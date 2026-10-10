// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import io.github.iamandelib.kpeek

// Scrollable page list for a PDF; shared by the PDF viewer and converted office documents.
Rectangle {
    id: root

    property alias url: document.url
    property real zoom: 1.0
    readonly property Flickable scrollTarget: view
    readonly property real fitWidth: Math.max(Kirigami.Units.gridUnit * 10, view.width - Kirigami.Units.largeSpacing * 2)
    readonly property real pageWidth: fitWidth * zoom
    // Render width in device pixels: rounded up to 256 px steps so resizing the window
    // doesn't re-render every page, and capped to bound memory when zoomed in.
    readonly property int renderWidth: Math.min(4096, Math.ceil(pageWidth * Screen.devicePixelRatio / 256) * 256)

    // Keep the point at the center of the view in place while zooming.
    property real lastZoom: 1.0
    onZoomChanged: {
        const ratio = zoom / lastZoom;
        lastZoom = zoom;
        const cx = view.contentX + view.width / 2;
        const cy = view.contentY - view.originY + view.height / 2;
        Qt.callLater(() => {
            view.contentX = Math.max(0, Math.min(cx * ratio - view.width / 2, view.contentWidth - view.width));
            view.contentY = view.originY + Math.max(0, Math.min(cy * ratio - view.height / 2, view.contentHeight - view.height));
        });
    }

    color: Kirigami.Theme.alternateBackgroundColor

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
                    sourceSize.width: root.renderWidth
                    asynchronous: true
                    cache: false
                    retainWhileLoading: true
                }
            }
        }
    }

    // Page position.
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
                property int current: 0
                // indexAt() is -1 in the gap between pages; keep the last page then.
                readonly property int probe: view.indexAt(view.contentX + view.width / 2, view.contentY + view.height / 2)
                onProbeChanged: if (probe >= 0) current = probe
                text: i18nc("@info page position", "Page %1 of %2", current + 1, document.pageCount)
                Layout.margins: Kirigami.Units.smallSpacing
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
