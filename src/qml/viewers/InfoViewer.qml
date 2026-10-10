// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import io.github.iamandelib.kpeek

// Fallback for folders and files without a dedicated viewer: a summary card.
Rectangle {
    color: Kirigami.Theme.backgroundColor

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - Kirigami.Units.gridUnit * 4, Kirigami.Units.gridUnit * 24)
        spacing: Kirigami.Units.largeSpacing

        Kirigami.Icon {
            source: PeekController.iconName
            fallback: PeekController.isFolder ? "folder" : "unknown"
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: Kirigami.Units.iconSizes.enormous
            Layout.preferredHeight: Kirigami.Units.iconSizes.enormous
        }

        Kirigami.Heading {
            text: PeekController.fileName
            level: 2
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
        }

        Kirigami.FormLayout {
            Layout.alignment: Qt.AlignHCenter

            QQC2.Label {
                Kirigami.FormData.label: i18nc("@label", "Type:")
                text: PeekController.mimeComment
            }
            QQC2.Label {
                Kirigami.FormData.label: PeekController.isFolder ? i18nc("@label", "Contains:") : i18nc("@label", "Size:")
                text: PeekController.sizeText
            }
            QQC2.Label {
                Kirigami.FormData.label: i18nc("@label", "Modified:")
                text: PeekController.modified.toLocaleString(Qt.locale(), Locale.ShortFormat)
            }
        }
    }
}
