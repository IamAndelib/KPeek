// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import io.github.iamandelib.kpeek

// Office documents are converted to PDF with LibreOffice, then shown like any PDF.
Rectangle {
    id: root

    property real zoom: 1.0
    readonly property Flickable scrollTarget: pdfLoader.item?.scrollTarget ?? null

    color: Kirigami.Theme.backgroundColor

    OfficeConverter {
        id: converter
        source: PeekController.url
    }

    Loader {
        id: pdfLoader
        anchors.fill: parent
        active: converter.status === OfficeConverter.Ready
        sourceComponent: PdfView {
            url: converter.pdfUrl
            zoom: root.zoom
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        visible: converter.status === OfficeConverter.Converting
        spacing: Kirigami.Units.largeSpacing

        QQC2.BusyIndicator {
            running: parent.visible
            Layout.alignment: Qt.AlignHCenter
        }
        QQC2.Label {
            text: i18nc("@info", "Preparing preview…")
            Layout.alignment: Qt.AlignHCenter
        }
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: converter.status === OfficeConverter.Unavailable || converter.status === OfficeConverter.Failed
        icon.name: PeekController.iconName
        text: converter.status === OfficeConverter.Unavailable ? i18nc("@info", "Install LibreOffice to preview this document") : i18nc("@info", "This document could not be previewed")
        explanation: converter.status === OfficeConverter.Unavailable ? i18nc("@info", "KPeek uses LibreOffice, installed from your distribution or Flathub, to display office documents.") : ""
        helpfulAction: QQC2.Action {
            icon.name: "document-open"
            text: i18nc("@action:button", "Open with Default Application")
            onTriggered: PeekController.openExternally()
        }
    }
}
