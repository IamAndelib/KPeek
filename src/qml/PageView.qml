// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import io.github.iamandelib.kpeek

// Scrollable page list shared by the PDF, office and comic viewers. The document is anything with
// pageCount, pageSize(i), pageSource(i) and failed (and optionally locked): PdfDocument, ComicBook.
Rectangle {
    id: root

    required property QtObject document
    property real zoom: 1.0
    readonly property Flickable scrollTarget: view
    readonly property bool dragPans: true
    readonly property real fitWidth: Math.max(Kirigami.Units.gridUnit * 10, view.width - Kirigami.Units.largeSpacing * 2)
    // Only ever set by layoutPages(), so that size and scroll position change in a known order.
    property real pageWidth: 0
    // Render width in device pixels: rounded up to 256 px steps so resizing the window
    // doesn't re-render every page, and capped to bound memory when zoomed in (an A4 page
    // 4096 px wide is 95 MB, and a few are alive at once).
    readonly property int renderWidth: Math.min(PeekController.lowMemory ? 2048 : 4096, Math.ceil(pageWidth * PeekController.devicePixelRatio / 256) * 256)

    // Keep the point under the view's centre in place when pages change size (zooming or resizing the
    // window). Everything happens here, in order: deferring the scroll position lets one frame show the
    // new size at the old position (flicker), and setting it before the content has grown gets it
    // clamped. Horizontally the page is centred in the content; vertically, pages scale but the gaps
    // between them don't, so the anchor is "page k, fraction t down that page".
    onZoomChanged: layoutPages()
    onFitWidthChanged: layoutPages()
    Component.onCompleted: layoutPages()

    function layoutPages() {
        const pageBefore = pageWidth;
        if (pageBefore <= 0) {
            pageWidth = fitWidth * zoom; // first layout: nothing to keep in place yet
            return;
        }
        const margin = Kirigami.Units.largeSpacing * 2;
        const contentBefore = Math.max(view.width, pageBefore + margin);
        const fx = (view.contentX + view.width / 2 - (contentBefore - pageBefore) / 2) / pageBefore;
        const centreY = view.contentY + view.height / 2;
        const index = view.indexAt(view.contentX + view.width / 2, centreY);
        const item = index >= 0 ? view.itemAtIndex(index) : null;
        const t = item ? (centreY - item.y) / item.height : 0;

        pageWidth = fitWidth * zoom; // resizes pages and content width right away
        // ListView repositions pages around the visible ones when their heights change, so lay out now
        // and read where the anchor page actually is, rather than predicting it.
        view.forceLayout();

        const contentAfter = Math.max(view.width, pageWidth + margin);
        view.contentX = Math.max(0, Math.min(fx * pageWidth + (contentAfter - pageWidth) / 2 - view.width / 2, contentAfter - view.width));
        if (item) {
            view.contentY = item.y + t * item.height - view.height / 2;
        }
    }

    readonly property color borderColor: Kirigami.ColorUtils.linearInterpolation(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.2)

    color: Kirigami.Theme.alternateBackgroundColor

    ListView {
        id: view
        anchors.fill: parent
        visible: root.document.pageCount > 0
        clip: true
        spacing: Kirigami.Units.largeSpacing
        topMargin: Kirigami.Units.largeSpacing
        bottomMargin: Kirigami.Units.gridUnit * 3
        contentWidth: Math.max(width, root.pageWidth + Kirigami.Units.largeSpacing * 2)
        flickableDirection: Flickable.AutoFlickIfNeeded
        boundsBehavior: Flickable.StopAtBounds
        cacheBuffer: height
        model: root.document.pageCount

        QQC2.ScrollBar.vertical: QQC2.ScrollBar {}
        QQC2.ScrollBar.horizontal: QQC2.ScrollBar {}

        delegate: Item {
            id: pageItem
            required property int index
            readonly property size pointSize: root.document.pageSize(index)

            width: view.contentWidth
            height: root.pageWidth * pointSize.height / Math.max(1, pointSize.width)

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                width: root.pageWidth
                height: parent.height
                color: "white"
                border.color: root.borderColor

                Image {
                    id: pageImage
                    anchors.fill: parent
                    source: root.document.pageSource(pageItem.index)
                    sourceSize.width: root.renderWidth
                    asynchronous: true
                    cache: false
                    retainWhileLoading: true
                }
                Kirigami.Icon {
                    anchors.centerIn: parent
                    width: Kirigami.Units.iconSizes.huge
                    height: width
                    visible: pageImage.status === Image.Error
                    source: "image-missing"
                }
            }
        }
    }

    // Page position.
    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Kirigami.Units.largeSpacing
        visible: root.document.pageCount > 0
        width: pageLabel.implicitWidth
        height: pageLabel.implicitHeight
        radius: Kirigami.Units.cornerRadius
        color: Qt.alpha(Kirigami.Theme.backgroundColor, 0.92)
        border.color: root.borderColor

        QQC2.Label {
            id: pageLabel
            property int current: 0
            // indexAt() is -1 in the gap between pages; keep the last page then.
            readonly property int probe: view.indexAt(view.contentX + view.width / 2, view.contentY + view.height / 2)
            onProbeChanged: if (probe >= 0) current = probe
            anchors.centerIn: parent
            padding: Kirigami.Units.smallSpacing * 2
            text: i18nc("@info page position", "Page %1 of %2", current + 1, root.document.pageCount)
        }
    }

    // Opening a large book or a document on a slow drive takes a moment.
    QQC2.BusyIndicator {
        anchors.centerIn: parent
        running: root.document.pageCount === 0 && !root.document.failed && !root.document.locked
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: root.document.failed || root.document.locked
        icon.name: root.document.locked ? "object-locked" : "dialog-error"
        text: root.document.locked ? i18nc("@info", "This document is password-protected") : i18nc("@info", "This document could not be opened")
    }
}
