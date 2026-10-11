// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

pragma ComponentBehavior: Bound

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
    // Rendering megabytes of Markdown would stall the window; such files are shown as source.
    readonly property bool sourceShown: showSource || !loader.highlight
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
            text: i18nc("@info %1 is a size such as 2 MiB", "This file is large; only the first %1 are shown.", Qt.locale().formattedDataSize(loader.maxBytes))
        }

        QQC2.ScrollView {
            id: scroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth // text wraps; never scroll sideways

            // Two separate components rather than one TextArea with a toggled highlighter,
            // which crashes KSyntaxHighlighting (see SourceText.qml).
            Loader {
                width: scroll.availableWidth
                height: item?.implicitHeight ?? 0
                sourceComponent: root.sourceShown ? sourceView : renderedView
            }
        }
    }

    Component {
        id: renderedView
        QQC2.TextArea {
            text: loader.markdownToHtml(loader.text, PeekController.url)
            textFormat: TextEdit.RichText
            readOnly: true
            selectByMouse: true
            wrapMode: TextEdit.Wrap
            font.pointSize: Kirigami.Theme.defaultFont.pointSize * root.zoom
            leftPadding: Kirigami.Units.gridUnit
            rightPadding: Kirigami.Units.gridUnit
            topPadding: Kirigami.Units.gridUnit
            background: null
            // Only web and mail links: a document must not hand an arbitrary scheme or a sibling file to its handler.
            onLinkActivated: link => {
                const url = new URL(link, PeekController.url);
                if (["http:", "https:", "mailto:"].includes(url.protocol)) {
                    Qt.openUrlExternally(url);
                }
            }
        }
    }

    Component {
        id: sourceView
        SourceText {
            text: loader.text
            fileName: PeekController.fileName
            highlight: loader.highlight
            wrap: true
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
        checked: root.sourceShown
        enabled: loader.highlight
        focusPolicy: Qt.NoFocus
        icon.name: "format-text-code"
        text: i18nc("@action:button", "Show Source")
        onToggled: root.showSource = checked
        QQC2.ToolTip.text: i18nc("@info:tooltip", "Show the Markdown source (Ctrl+U)")
        QQC2.ToolTip.visible: hovered
    }

    Shortcut {
        sequence: "Ctrl+U" // "view source", as in web browsers
        enabled: loader.highlight
        onActivated: root.showSource = !root.showSource
    }

    RelativeScrollAnchor {
        flickable: root.scrollTarget
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: loader.failed || loader.text.length === 0
        icon.name: loader.failed ? "dialog-error" : PeekController.iconName
        text: loader.failed ? i18nc("@info", "This file could not be read") : i18nc("@info", "This file is empty")
    }
}
