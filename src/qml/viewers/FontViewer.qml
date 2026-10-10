// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import io.github.iamandelib.kpeek

// Font specimen: the family name, the alphabet and a pangram at several sizes.
Rectangle {
    id: root

    property real zoom: 1.0
    readonly property Flickable scrollTarget: scroll.contentItem as Flickable

    color: Kirigami.Theme.backgroundColor

    FontFile {
        id: loader
        url: PeekController.url
    }

    QQC2.ScrollView {
        id: scroll
        contentWidth: availableWidth // samples elide; never scroll sideways
        anchors.fill: parent
        visible: !loader.failed

        ColumnLayout {
            width: scroll.availableWidth
            spacing: Kirigami.Units.largeSpacing

            Kirigami.Heading {
                text: [loader.font.family, loader.font.styleName].filter(s => s.length > 0).join(" ")
                textFormat: Text.PlainText // names come from the font file
                level: 1
                wrapMode: Text.Wrap
                Layout.fillWidth: true
                Layout.margins: Kirigami.Units.gridUnit
            }

            QQC2.Label {
                text: "ABCDEFGHIJKLMNOPQRSTUVWXYZ\nabcdefghijklmnopqrstuvwxyz\n0123456789 .,;:!?&@#%()[]{}"
                font.family: loader.font.family
                font.styleName: loader.font.styleName
                font.pointSize: 18 * root.zoom
                wrapMode: Text.Wrap
                Layout.fillWidth: true
                Layout.leftMargin: Kirigami.Units.gridUnit
                Layout.rightMargin: Kirigami.Units.gridUnit
            }

            Repeater {
                model: [10, 14, 20, 28, 40, 56]

                QQC2.Label {
                    required property int modelData
                    text: i18nc("@info font sample pangram", "The quick brown fox jumps over the lazy dog")
                    font.family: loader.font.family
                    font.styleName: loader.font.styleName
                    font.pointSize: modelData * root.zoom
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                    Layout.leftMargin: Kirigami.Units.gridUnit
                    Layout.rightMargin: Kirigami.Units.gridUnit
                }
            }
        }
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: loader.failed
        icon.name: "dialog-error"
        text: i18nc("@info", "This font could not be loaded")
    }

    RelativeScrollAnchor {
        flickable: root.scrollTarget
    }
}
