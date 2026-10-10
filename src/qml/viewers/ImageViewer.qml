// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kpeek

Rectangle {
    color: Kirigami.Theme.backgroundColor

    // AnimatedImage plays GIF/WebP/APNG animations; other formats load through Image,
    // which supports everything QImageReader and KImageFormats can decode.
    readonly property bool animated: ["image/gif", "image/webp", "image/apng"].includes(PeekController.mimeType)

    Loader {
        id: imageLoader
        anchors.fill: parent
        anchors.margins: Kirigami.Units.smallSpacing
        sourceComponent: parent.animated ? animatedComponent : staticComponent
    }

    Component {
        id: staticComponent
        Image {
            source: PeekController.url
            fillMode: Image.PreserveAspectFit
            asynchronous: true
            autoTransform: true
            smooth: true
            mipmap: true
            // Decode at display size instead of full resolution; keeps huge photos fast.
            sourceSize.width: Math.max(width, 1) * Screen.devicePixelRatio
            sourceSize.height: Math.max(height, 1) * Screen.devicePixelRatio
        }
    }

    Component {
        id: animatedComponent
        AnimatedImage {
            source: PeekController.url
            fillMode: Image.PreserveAspectFit
            asynchronous: true
            smooth: true
        }
    }

    QQC2.BusyIndicator {
        anchors.centerIn: parent
        running: imageLoader.item && imageLoader.item.status === Image.Loading
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: imageLoader.item && imageLoader.item.status === Image.Error
        icon.name: "image-missing"
        text: i18nc("@info", "This image could not be displayed")
    }
}
