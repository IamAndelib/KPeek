// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import io.github.iamandelib.kpeek

// Office documents are converted to PDF with LibreOffice, then shown like any PDF. Until then, and
// always in the Flatpak (which can't reach LibreOffice), the preview picture stored in the document
// is shown, or Dolphin's thumbnail of it.
Rectangle {
    id: root

    property real zoom: 1.0
    readonly property Flickable scrollTarget: pdfLoader.item?.scrollTarget ?? null
    readonly property bool dragPans: true
    readonly property bool converting: converter.status === OfficeConverter.Converting

    color: Kirigami.Theme.backgroundColor

    OfficeConverter {
        id: converter
        source: PeekController.url
    }

    Loader {
        id: pdfLoader
        anchors.fill: parent
        active: converter.status === OfficeConverter.Ready
        sourceComponent: PageView {
            document: PdfDocument {
                url: converter.pdfUrl
            }
            zoom: root.zoom
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Kirigami.Units.largeSpacing
        visible: !pdfLoader.active && preview.status === Image.Ready
        spacing: Kirigami.Units.largeSpacing

        Image {
            id: preview
            Layout.fillWidth: true
            Layout.fillHeight: true
            fillMode: Image.PreserveAspectFit
            asynchronous: true
            // The document's own preview first; if it has none or it fails, Dolphin's thumbnail.
            source: converter.embeddedPreview.length > 0 && !embeddedFailed ? converter.embeddedPreview : PeekController.thumbnail
            property bool embeddedFailed: false
            onStatusChanged: if (status === Image.Error && source.toString() === converter.embeddedPreview) embeddedFailed = true
        }
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: Kirigami.Units.smallSpacing

            QQC2.BusyIndicator {
                visible: root.converting
                running: visible
                Layout.preferredHeight: Kirigami.Units.iconSizes.medium
            }
            QQC2.Label {
                text: root.converting ? i18nc("@info", "Preparing the full preview…") : i18nc("@info", "First page only.")
                opacity: 0.7
            }
            QQC2.Button {
                visible: !root.converting
                icon.name: "document-open"
                text: i18nc("@action:button", "Open")
                focusPolicy: Qt.NoFocus
                onClicked: PeekController.openExternally()
            }
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        visible: root.converting && preview.status !== Image.Ready
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
        visible: (converter.status === OfficeConverter.Unavailable || converter.status === OfficeConverter.Failed) && preview.status !== Image.Ready
                 && preview.status !== Image.Loading
        icon.name: PeekController.iconName
        text: converter.status === OfficeConverter.Unavailable && !converter.sandboxed ? i18nc("@info", "Install LibreOffice to preview this document")
                                                                                      : i18nc("@info", "This document has no preview")
        explanation: converter.status === OfficeConverter.Unavailable && !converter.sandboxed
                     ? i18nc("@info", "KPeek uses LibreOffice, installed from your distribution or Flathub, to display office documents.")
                     : ""
        helpfulAction: QQC2.Action {
            icon.name: "document-open"
            text: i18nc("@action:button", "Open with Default Application")
            onTriggered: PeekController.openExternally()
        }
    }
}
