// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import io.github.iamandelib.kpeek

/**
 * Fitted to the window at zoom 1 (never upscaled); zooming in makes it pannable.
 *
 * Three layers keep this fast and frugal whatever the image size:
 *  1. Dolphin's cached thumbnail, shown instantly while the image decodes;
 *  2. the base: the whole image decoded once at screen size, never again while zooming (no flashing);
 *  3. a detail tile: when zoomed in past what the base resolves, just the visible part, decoded at the
 *     current zoom once zooming or panning settles (ImageTiles in C++). Memory stays at about two screens.
 */
Rectangle {
    id: root

    property real zoom: 1.0
    readonly property Flickable scrollTarget: flick
    readonly property bool dragPans: true

    // Natural size in logical pixels (from the file header, or the decoded image for formats
    // that can't report it up front), and the size it is shown at.
    readonly property size pixelSize: PeekController.imageSize.width > 0 ? PeekController.imageSize
        : Qt.size(base.item?.implicitWidth ?? 0, base.item?.implicitHeight ?? 0)
    readonly property size naturalSize: Qt.size(pixelSize.width / Screen.devicePixelRatio, pixelSize.height / Screen.devicePixelRatio)
    readonly property real fitScale: naturalSize.width > 0 && naturalSize.height > 0
        ? Math.min(1, flick.width / naturalSize.width, flick.height / naturalSize.height) : 1
    readonly property real shownWidth: naturalSize.width * fitScale * zoom
    readonly property real shownHeight: naturalSize.height * fitScale * zoom

    readonly property bool baseReady: base.item?.status === Image.Ready
    // Detail tiles only help once the image is shown larger than the base decode.
    readonly property bool needsTiles: PeekController.tileable && baseReady
        && shownWidth * Screen.devicePixelRatio > (base.item?.implicitWidth ?? 0) * 1.05

    // Keep the point at the center of the view in place while zooming.
    property real lastZoom: 1.0
    onZoomChanged: {
        const ratio = zoom / lastZoom;
        lastZoom = zoom;
        const cx = flick.contentX + flick.width / 2;
        const cy = flick.contentY + flick.height / 2;
        Qt.callLater(() => {
            flick.contentX = Math.max(0, Math.min(cx * ratio - flick.width / 2, flick.contentWidth - flick.width));
            flick.contentY = Math.max(0, Math.min(cy * ratio - flick.height / 2, flick.contentHeight - flick.height));
        });
        tileTimer.restart();
    }

    function requestTile() {
        if (!needsTiles) {
            tile.source = "";
            return;
        }
        // The visible part of the image, in logical pixels relative to the image.
        const left = Math.max(0, flick.contentX - canvas.x);
        const top = Math.max(0, flick.contentY - canvas.y);
        const right = Math.min(shownWidth, flick.contentX + flick.width - canvas.x);
        const bottom = Math.min(shownHeight, flick.contentY + flick.height - canvas.y);
        if (right <= left || bottom <= top) {
            return;
        }
        const dpr = Screen.devicePixelRatio;
        const clip = Qt.rect(Math.floor(left * dpr), Math.floor(top * dpr), Math.ceil((right - left) * dpr), Math.ceil((bottom - top) * dpr));
        tile.x = clip.x / dpr;
        tile.y = clip.y / dpr;
        tile.width = clip.width / dpr;
        tile.height = clip.height / dpr;
        tile.zoom = zoom;
        tile.source = PeekController.tileSource(Qt.size(Math.round(shownWidth * dpr), Math.round(shownHeight * dpr)), clip);
    }

    color: Kirigami.Theme.backgroundColor

    // Ask for a new tile once zooming, panning or resizing has paused.
    Timer {
        id: tileTimer
        interval: 150
        onTriggered: root.requestTile()
    }
    Connections {
        target: flick
        function onContentXChanged() { tileTimer.restart(); }
        function onContentYChanged() { tileTimer.restart(); }
        function onWidthChanged() { tileTimer.restart(); }
        function onHeightChanged() { tileTimer.restart(); }
    }
    onNeedsTilesChanged: tileTimer.restart()

    Flickable {
        id: flick
        anchors.fill: parent
        anchors.margins: Kirigami.Units.smallSpacing
        contentWidth: Math.max(width, root.shownWidth)
        contentHeight: Math.max(height, root.shownHeight)
        interactive: contentWidth > width || contentHeight > height
        boundsBehavior: Flickable.StopAtBounds
        clip: true

        Item {
            id: canvas
            x: (flick.contentWidth - width) / 2
            y: (flick.contentHeight - height) / 2
            width: root.shownWidth
            height: root.shownHeight

            Image {
                anchors.fill: parent
                visible: !root.baseReady
                source: PeekController.thumbnail
                fillMode: Image.PreserveAspectFit
                asynchronous: true
                smooth: true
            }

            Loader {
                id: base
                anchors.fill: parent
                sourceComponent: PeekController.animated ? animatedComponent : staticComponent
            }

            Image {
                id: tile
                property real zoom
                visible: root.needsTiles && status === Image.Ready && zoom === root.zoom
                asynchronous: true
                cache: false
                smooth: true
            }
        }

        QQC2.ScrollBar.vertical: QQC2.ScrollBar {}
        QQC2.ScrollBar.horizontal: QQC2.ScrollBar {}
    }

    Component {
        id: staticComponent
        Image {
            source: PeekController.url
            fillMode: Image.PreserveAspectFit
            asynchronous: true
            autoTransform: true
            smooth: true
            // Decoded once, at screen size, never the full resolution of huge photos.
            sourceSize: Qt.size(Screen.width * Screen.devicePixelRatio, Screen.height * Screen.devicePixelRatio)
        }
    }

    Component {
        id: animatedComponent
        AnimatedImage {
            source: PeekController.url
            fillMode: Image.PreserveAspectFit
            asynchronous: true
            smooth: true
            cache: false // otherwise every decoded frame stays in memory
        }
    }

    QQC2.BusyIndicator {
        anchors.centerIn: parent
        running: base.item?.status === Image.Loading && PeekController.thumbnail.toString() === ""
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: base.item?.status === Image.Error
        icon.name: "image-missing"
        text: i18nc("@info", "This image could not be displayed")
    }
}
