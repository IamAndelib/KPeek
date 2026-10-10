// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kpeek

// Contents listing of zip, tar, 7z and ar archives.
Rectangle {
    color: Kirigami.Theme.backgroundColor
    focus: true
    Keys.onPressed: event => ScrollKeys.handle(event, list)

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
            text: i18ncp("@info archive summary", "%1 file, %2 uncompressed", "%1 files, %2 uncompressed", archive.fileCount, archive.totalSizeText)
                  + (archive.truncated ? " " + i18nc("@info", "(only the first %1 entries are listed)", archive.maxEntries) : "")
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

                delegate: QQC2.ItemDelegate {
                    required property string name
                    required property string path
                    required property int depth
                    required property string sizeText
                    required property string iconName

                    width: ListView.view.width
                    hoverEnabled: false
                    down: false
                    focusPolicy: Qt.NoFocus
                    QQC2.ToolTip.text: path
                    QQC2.ToolTip.visible: hovered && nameLabel.truncated

                    contentItem: RowLayout {
                        spacing: Kirigami.Units.smallSpacing

                        Kirigami.Icon {
                            source: iconName
                            fallback: "unknown"
                            Layout.leftMargin: depth * Kirigami.Units.gridUnit
                            Layout.preferredWidth: Kirigami.Units.iconSizes.small
                            Layout.preferredHeight: Kirigami.Units.iconSizes.small
                        }
                        QQC2.Label {
                            id: nameLabel
                            text: name
                            elide: Text.ElideMiddle
                            Layout.fillWidth: true
                        }
                        QQC2.Label {
                            text: sizeText
                            opacity: 0.7
                        }
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
