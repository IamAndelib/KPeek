// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import io.github.iamandelib.kpeek

// Fallback for folders and files without a dedicated viewer: a summary card. When Dolphin has made a
// thumbnail (camera RAW, EPUB, Blender, Krita, …), it is shown instead of the generic icon.
Rectangle {
    id: root

    color: Kirigami.Theme.backgroundColor

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - Kirigami.Units.gridUnit * 4, Kirigami.Units.gridUnit * 24)
        spacing: Kirigami.Units.largeSpacing

        Image {
            id: thumbnail
            visible: status === Image.Ready
            source: PeekController.thumbnail
            fillMode: Image.PreserveAspectFit
            asynchronous: true
            smooth: true
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: Math.min(implicitWidth, root.width * 0.6)
            Layout.preferredHeight: Math.min(implicitHeight, root.height * 0.5)
        }

        Kirigami.Icon {
            visible: !thumbnail.visible
            source: PeekController.iconName
            fallback: PeekController.isFolder ? "folder" : "unknown"
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: Kirigami.Units.iconSizes.enormous
            Layout.preferredHeight: Kirigami.Units.iconSizes.enormous
        }

        Kirigami.Heading {
            text: PeekController.fileName
            textFormat: Text.PlainText
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
