// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import io.github.iamandelib.kpeek

/**
 * Fitted to the window at zoom 1 (never upscaled); zooming in makes it pannable.
 *
 * Like GNOME Sushi and Gwenview: decode once, let the GPU scale while zooming, and never hide pixels
 * before something better is ready to replace them. Three stacked layers:
 *  1. Dolphin's cached thumbnail, shown instantly while the image decodes;
 *  2. the base: the whole image decoded once at screen size, which is all most previews ever need;
 *  3. full detail: the first time zooming needs more pixels than the base has, the image is decoded
 *     once more at its own size (at most 8192 px). It appears on top only
 *     when ready; until then the base keeps showing, scaled. After that, zooming is pure GPU scaling.
 */
Rectangle {
    id: root

    property real zoom: 1.0
    readonly property Flickable scrollTarget: flick
    readonly property bool dragPans: true

    // Natural size in logical pixels (from the file header, or the decoded image for formats
    // that can't report it up front), and the size it is shown at.
    readonly property size pixelSize: PeekController.imageSize.width > 0 ? PeekController.imageSize
        : Qt.size(baseImage?.implicitWidth ?? 0, baseImage?.implicitHeight ?? 0)
    readonly property size naturalSize: Qt.size(pixelSize.width / PeekController.devicePixelRatio, pixelSize.height / PeekController.devicePixelRatio)
    readonly property real fitScale: naturalSize.width > 0 && naturalSize.height > 0
        ? Math.min(1, flick.width / naturalSize.width, flick.height / naturalSize.height) : 1
    // Zoom is relative to the fitted size: allow at least 200% of the real pixels (up to 6400%).
    readonly property real maxZoom: Math.min(64, Math.max(4, 2 / fitScale))
    readonly property real shownWidth: naturalSize.width * fitScale * zoom
    readonly property real shownHeight: naturalSize.height * fitScale * zoom

    // A decode size for an image of pixelSize to fit within `box`, or undefined (decode as is) when it
    // already fits: Image.sourceSize also scales *up* to the box, which would waste memory and sharpness.
    // Uses the size from the file header, never the decoded size: that would make the decode size
    // depend on its own result. Unknown sizes get the box.
    function decodeSize(box) {
        const size = PeekController.imageSize;
        const fits = size.width > 0 && size.width <= box.width && size.height <= box.height;
        return fits ? undefined : box;
    }

    // Also within the GPU's texture limit (4096 or 8192 on older integrated GPUs), past which Qt
    // would shrink the image again on the CPU before every upload.
    readonly property int detailCap: Math.min(PeekController.lowMemory ? 4096 : 8192, PeekController.maxTextureSize)

    // The loaders' items, typed (AnimatedImage is an Image too).
    readonly property Image baseImage: base.item as Image
    readonly property Image detailImage: detail.item as Image
    readonly property bool baseReady: baseImage?.status === Image.Ready
    // Set (for good) the first time the image is shown larger than the base decode resolves.
    property bool wantsDetail: false
    readonly property bool baseTooSmall: !PeekController.animated && baseReady
        && shownWidth * PeekController.devicePixelRatio > (baseImage?.implicitWidth ?? 0) * 1.05
    // Above 256 MP even the reduced decode would exceed Qt's image allocation limit and fail, so such
    // images zoom on the base alone.
    readonly property bool detailDecodable: pixelSize.width * pixelSize.height <= 256e6
    onBaseTooSmallChanged: if (baseTooSmall && detailDecodable) wantsDetail = true
    // Exactly one of base and detail is drawn: detail once loaded and while zoomed past the base,
    // the base otherwise (also when zooming back out, so no mipmaps are needed for clean downscaling).
    readonly property bool detailShown: detailImage?.status === Image.Ready && baseTooSmall

    // Keep the image point under the view's centre in place while zooming. The new position is
    // computed from the zoom formula and set in this same handler: deferring it (or reading bindings
    // that may not have updated yet) lets one frame show the new size at the old position, which
    // flickers whenever the image is larger than the view.
    property real lastZoom: 1.0
    onZoomChanged: {
        const before = Qt.size(naturalSize.width * fitScale * lastZoom, naturalSize.height * fitScale * lastZoom);
        const after = Qt.size(naturalSize.width * fitScale * zoom, naturalSize.height * fitScale * zoom);
        lastZoom = zoom;
        if (before.width <= 0 || before.height <= 0) {
            return;
        }
        // The canvas is centred in the content when smaller than the view (see canvas.x/y).
        const fx = (flick.contentX + flick.width / 2 - Math.max(0, flick.width - before.width) / 2) / before.width;
        const fy = (flick.contentY + flick.height / 2 - Math.max(0, flick.height - before.height) / 2) / before.height;
        const contentWidth = Math.max(flick.width, after.width);
        const contentHeight = Math.max(flick.height, after.height);
        flick.contentX = Math.max(0, Math.min(fx * after.width + (contentWidth - after.width) / 2 - flick.width / 2, contentWidth - flick.width));
        flick.contentY = Math.max(0, Math.min(fy * after.height + (contentHeight - after.height) / 2 - flick.height / 2, contentHeight - flick.height));
    }

    color: Kirigami.Theme.backgroundColor

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
                visible: !root.baseReady && baseImage?.status !== Image.Error
                source: PeekController.thumbnail
                fillMode: Image.PreserveAspectFit
                asynchronous: true
                smooth: true
            }

            Loader {
                id: base
                anchors.fill: parent
                visible: !root.detailShown
                sourceComponent: PeekController.animated ? animatedComponent : staticComponent
            }

            Loader {
                id: detail
                anchors.fill: parent
                active: root.wantsDetail
                sourceComponent: Image {
                    visible: root.detailShown
                    source: PeekController.url
                    fillMode: Image.PreserveAspectFit
                    asynchronous: true
                    autoTransform: true
                    smooth: true
                    // At most 8192 px a side (up to 67 MP), 4096 on low-memory machines, and within the
                    // GPU's texture limit (detailCap).
                    sourceSize: root.decodeSize(Qt.size(root.detailCap, root.detailCap))
                }
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
            // Decoded once, at most at screen size: never the full resolution of huge photos.
            sourceSize: root.decodeSize(Qt.size(Math.min(Screen.width * PeekController.devicePixelRatio, root.detailCap),
                                                Math.min(Screen.height * PeekController.devicePixelRatio, root.detailCap)))
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
        running: baseImage?.status === Image.Loading && PeekController.thumbnail.toString() === ""
    }

    // A file that can't be decoded here (e.g. a camera RAW format this system lacks a decoder for)
    // still gets Dolphin's thumbnail if there is one; the error message is the last resort.
    Image {
        id: fallback
        anchors.centerIn: parent
        width: Math.min(implicitWidth, parent.width - Kirigami.Units.gridUnit * 2)
        height: Math.min(implicitHeight, parent.height - Kirigami.Units.gridUnit * 2)
        visible: baseImage?.status === Image.Error && status === Image.Ready
        source: baseImage?.status === Image.Error ? PeekController.thumbnail : ""
        fillMode: Image.PreserveAspectFit
        asynchronous: true
        smooth: true
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: baseImage?.status === Image.Error && !fallback.visible && fallback.status !== Image.Loading
        icon.name: "image-missing"
        text: i18nc("@info", "This image could not be displayed")
    }
}
