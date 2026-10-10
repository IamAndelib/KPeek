// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

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
 *     once more at its own size (at most 8192 px, within GPU texture limits). It appears on top only
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
        : Qt.size(base.item?.implicitWidth ?? 0, base.item?.implicitHeight ?? 0)
    readonly property size naturalSize: Qt.size(pixelSize.width / Screen.devicePixelRatio, pixelSize.height / Screen.devicePixelRatio)
    readonly property real fitScale: naturalSize.width > 0 && naturalSize.height > 0
        ? Math.min(1, flick.width / naturalSize.width, flick.height / naturalSize.height) : 1
    readonly property real shownWidth: naturalSize.width * fitScale * zoom
    readonly property real shownHeight: naturalSize.height * fitScale * zoom

    // A decode size for an image of pixelSize to fit within `box`, or undefined (decode as is) when it
    // already fits: Image.sourceSize also scales *up* to the box, which would waste memory and sharpness.
    function decodeSize(box) {
        const fits = pixelSize.width > 0 && pixelSize.width <= box.width && pixelSize.height <= box.height;
        return fits ? undefined : box;
    }

    readonly property bool baseReady: base.item?.status === Image.Ready
    // Set (for good) the first time the image is shown larger than the base decode resolves.
    property bool wantsDetail: false
    readonly property bool baseTooSmall: !PeekController.animated && baseReady
        && shownWidth * Screen.devicePixelRatio > (base.item?.implicitWidth ?? 0) * 1.05
    // Above 256 MP even the reduced decode would exceed Qt's image allocation limit and fail, so such
    // images zoom on the base alone.
    readonly property bool detailDecodable: pixelSize.width * pixelSize.height <= 256e6
    onBaseTooSmallChanged: if (baseTooSmall && detailDecodable) wantsDetail = true

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

            Loader {
                anchors.fill: parent
                active: root.wantsDetail
                sourceComponent: Image {
                    visible: status === Image.Ready // the base stays visible underneath until then
                    source: PeekController.url
                    fillMode: Image.PreserveAspectFit
                    asynchronous: true
                    autoTransform: true
                    smooth: true
                    mipmap: true // clean downscaling when zoomed back out
                    // At most 8192 px a side: within every GPU's texture limit and ~45 MP of memory.
                    sourceSize: root.decodeSize(Qt.size(8192, 8192))
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
            sourceSize: root.decodeSize(Qt.size(Screen.width * Screen.devicePixelRatio, Screen.height * Screen.devicePixelRatio))
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
