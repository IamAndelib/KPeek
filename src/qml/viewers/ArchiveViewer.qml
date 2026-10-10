// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import io.github.iamandelib.kpeek

// Contents listing of zip, tar, 7z and ar archives.
Rectangle {
    readonly property Flickable scrollTarget: list

    color: Kirigami.Theme.backgroundColor

    ArchiveModel {
        id: archive
        url: PeekController.url
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        visible: !archive.loading && !archive.failed

        QQC2.Label {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing
            text: archive.truncated
                ? i18ncp("@info archive summary", "First %1 entry listed; the archive has more", "First %1 entries listed; the archive has more", archive.maxEntries)
                : i18ncp("@info archive summary", "%1 file, %2 uncompressed", "%1 files, %2 uncompressed", archive.fileCount, archive.totalSizeText)
            opacity: 0.7
        }

        Kirigami.Separator {
            Layout.fillWidth: true
        }

        QQC2.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: list
                clip: true
                model: archive
                reuseItems: true

                // A plain row rather than an ItemDelegate: entries aren't clickable, and archives can hold thousands.
                delegate: RowLayout {
                    id: row

                    required property string name
                    required property string path
                    required property int depth
                    required property string sizeText
                    required property string iconName

                    width: ListView.view.width
                    height: Kirigami.Units.gridUnit * 1.6
                    spacing: Kirigami.Units.smallSpacing

                    Kirigami.Icon {
                        source: row.iconName
                        fallback: "unknown"
                        Layout.leftMargin: Kirigami.Units.largeSpacing + row.depth * Kirigami.Units.gridUnit
                        Layout.preferredWidth: Kirigami.Units.iconSizes.small
                        Layout.preferredHeight: Kirigami.Units.iconSizes.small
                    }
                    QQC2.Label {
                        id: nameLabel
                        text: row.name
                        elide: Text.ElideMiddle
                        Layout.fillWidth: true

                        HoverHandler {
                            id: hover
                        }
                        QQC2.ToolTip.text: row.path
                        QQC2.ToolTip.visible: hover.hovered && nameLabel.truncated
                    }
                    QQC2.Label {
                        text: row.sizeText
                        opacity: 0.7
                        Layout.rightMargin: Kirigami.Units.largeSpacing
                    }
                }
            }
        }
    }

    QQC2.BusyIndicator {
        anchors.centerIn: parent
        running: archive.loading
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: archive.failed
        icon.name: "dialog-error"
        text: i18nc("@info", "This archive could not be read")
    }
}
