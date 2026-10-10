// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.syntaxhighlighting
import org.kde.kpeek

// Rendered Markdown, with a toggle to show the source.
Rectangle {
    id: root

    property bool showSource: false

    color: Kirigami.Theme.backgroundColor
    focus: true
    Keys.onPressed: event => ScrollKeys.handle(event, scroll.contentItem)

    TextLoader {
        id: loader
        url: PeekController.url
    }

    QQC2.ScrollView {
        id: scroll
        anchors.fill: parent

        QQC2.TextArea {
            id: area
            text: loader.text
            textFormat: root.showSource ? TextEdit.PlainText : TextEdit.MarkdownText
            readOnly: true
            selectByMouse: true
            wrapMode: TextEdit.Wrap
            font: root.showSource ? Kirigami.Theme.fixedWidthFont : Kirigami.Theme.defaultFont
            leftPadding: Kirigami.Units.gridUnit
            rightPadding: Kirigami.Units.gridUnit
            topPadding: Kirigami.Units.gridUnit
            background: null
            onLinkActivated: link => Qt.openUrlExternally(link)
        }
    }

    SyntaxHighlighter {
        textEdit: root.showSource ? area : null
        definition: Repository.definitionForName("Markdown")
        theme: Repository.defaultTheme(Kirigami.ColorUtils.brightnessForColor(Kirigami.Theme.backgroundColor) === Kirigami.ColorUtils.Dark ? Repository.DarkTheme : Repository.LightTheme)
    }

    QQC2.ToolButton {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: Kirigami.Units.largeSpacing
        checkable: true
        checked: root.showSource
        icon.name: "format-text-code"
        text: i18nc("@action:button", "Show Source")
        onToggled: root.showSource = checked
    }
}
