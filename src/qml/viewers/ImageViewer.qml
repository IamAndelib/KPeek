// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import io.github.iamandelib.kpeek

// Fitted to the window at zoom 1 (never upscaled); zooming in makes it pannable.
Rectangle {
    id: root

    property real zoom: 1.0
    readonly property Flickable scrollTarget: flick

    // Natural size in logical pixels (from the file header, or the decoded image for formats
    // that can't report it up front), and the size it is shown at.
    readonly property size pixelSize: PeekController.imageSize.width > 0 ? PeekController.imageSize
        : Qt.size(imageLoader.item?.implicitWidth ?? 0, imageLoader.item?.implicitHeight ?? 0)
    readonly property size naturalSize: Qt.size(pixelSize.width / Screen.devicePixelRatio, pixelSize.height / Screen.devicePixelRatio)
    readonly property real fitScale: naturalSize.width > 0 && naturalSize.height > 0
        ? Math.min(1, flick.width / naturalSize.width, flick.height / naturalSize.height) : 1
    readonly property real shownWidth: naturalSize.width * fitScale * zoom
    readonly property real shownHeight: naturalSize.height * fitScale * zoom

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

        Loader {
            id: imageLoader
            x: (flick.contentWidth - width) / 2
            y: (flick.contentHeight - height) / 2
            width: root.shownWidth
            height: root.shownHeight
            sourceComponent: PeekController.animated ? animatedComponent : staticComponent
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
            retainWhileLoading: true
            // Decode at screen size (more only once zoomed in), never the full resolution of huge photos.
            readonly property int detail: root.zoom > 2 ? 4 : root.zoom > 1 ? 2 : 1
            sourceSize: Qt.size(Screen.width * Screen.devicePixelRatio * detail, Screen.height * Screen.devicePixelRatio * detail)
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
        running: imageLoader.item?.status === Image.Loading
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: imageLoader.item?.status === Image.Error
        icon.name: "image-missing"
        text: i18nc("@info", "This image could not be displayed")
    }
}
