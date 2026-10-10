// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibsriz@proton.me>

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kpeek

// Font specimen: the family name, the alphabet and a pangram at several sizes.
Rectangle {
    color: Kirigami.Theme.backgroundColor
    focus: true
    Keys.onPressed: event => ScrollKeys.handle(event, scroll.contentItem)

    FontLoader {
        id: loader
        source: PeekController.url
    }

    QQC2.ScrollView {
        id: scroll
        anchors.fill: parent
        visible: loader.status === FontLoader.Ready

        ColumnLayout {
            width: scroll.availableWidth
            spacing: Kirigami.Units.largeSpacing

            Kirigami.Heading {
                text: [loader.font.family, loader.font.styleName].filter(s => s.length > 0).join(" ")
                level: 1
                wrapMode: Text.Wrap
                Layout.fillWidth: true
                Layout.margins: Kirigami.Units.gridUnit
            }

            QQC2.Label {
                text: "ABCDEFGHIJKLMNOPQRSTUVWXYZ\nabcdefghijklmnopqrstuvwxyz\n0123456789 .,;:!?&@#%()[]{}"
                font.family: loader.font.family
                font.styleName: loader.font.styleName
                font.pointSize: 18
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
                    font.pointSize: modelData
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
        visible: loader.status === FontLoader.Error
        icon.name: "dialog-error"
        text: i18nc("@info", "This font could not be loaded")
    }
}
