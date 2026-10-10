// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import io.github.iamandelib.kpeek

// Plain text and source code with syntax highlighting.
Rectangle {
    id: root

    property real zoom: 1.0
    readonly property Flickable scrollTarget: scroll.contentItem as Flickable

    color: Kirigami.Theme.backgroundColor

    TextLoader {
        id: loader
        url: PeekController.url
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.smallSpacing
            visible: loader.truncated
            type: Kirigami.MessageType.Information
            text: i18nc("@info", "This file is large; only the first 2 MiB are shown.")
        }

        QQC2.ScrollView {
            id: scroll
            Layout.fillWidth: true
            Layout.fillHeight: true

            SourceText {
                text: loader.text
                fileName: PeekController.fileName
                highlight: loader.highlight
                zoom: root.zoom
            }
        }
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: loader.failed
        icon.name: "dialog-error"
        text: i18nc("@info", "This file could not be read")
    }
}
