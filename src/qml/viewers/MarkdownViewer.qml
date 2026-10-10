// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import io.github.iamandelib.kpeek

// Rendered Markdown, with a toggle to show the source.
Rectangle {
    id: root

    property real zoom: 1.0
    property bool showSource: false
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

            // Two separate components rather than one TextArea with a toggled highlighter,
            // which crashes KSyntaxHighlighting (see SourceText.qml).
            Loader {
                width: scroll.availableWidth
                height: item?.implicitHeight ?? 0
                sourceComponent: root.showSource ? sourceView : renderedView
            }
        }
    }

    Component {
        id: renderedView
        QQC2.TextArea {
            text: loader.text
            textFormat: TextEdit.MarkdownText
            baseUrl: PeekController.url // resolves relative images and links
            readOnly: true
            selectByMouse: true
            wrapMode: TextEdit.Wrap
            font.pointSize: Kirigami.Theme.defaultFont.pointSize * root.zoom
            leftPadding: Kirigami.Units.gridUnit
            rightPadding: Kirigami.Units.gridUnit
            topPadding: Kirigami.Units.gridUnit
            background: null
            onLinkActivated: link => Qt.openUrlExternally(new URL(link, PeekController.url))
        }
    }

    Component {
        id: sourceView
        SourceText {
            text: loader.text
            fileName: PeekController.fileName
            highlight: loader.highlight
            zoom: root.zoom
        }
    }

    QQC2.ToolButton {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: Kirigami.Units.largeSpacing
        // Keep clear of the vertical scrollbar.
        anchors.rightMargin: Kirigami.Units.largeSpacing + (scroll.QQC2.ScrollBar.vertical?.width ?? 0)
        checkable: true
        checked: root.showSource
        focusPolicy: Qt.NoFocus
        icon.name: "format-text-code"
        text: i18nc("@action:button", "Show Source")
        onToggled: root.showSource = checked
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: loader.failed
        icon.name: "dialog-error"
        text: i18nc("@info", "This file could not be read")
    }
}
