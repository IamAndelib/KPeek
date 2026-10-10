// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibsriz@proton.me>

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.syntaxhighlighting
import org.kde.kpeek

// Plain text and source code with syntax highlighting.
Rectangle {
    id: root

    // Highlighting megabytes of text takes seconds; beyond this the text is shown plain.
    readonly property int highlightLimit: 512 * 1024

    color: Kirigami.Theme.backgroundColor
    focus: true
    Keys.onPressed: event => ScrollKeys.handle(event, scroll.contentItem)

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

            QQC2.TextArea {
                id: area
                text: loader.text
                textFormat: TextEdit.PlainText
                readOnly: true
                selectByMouse: true
                wrapMode: TextEdit.Wrap
                font: Kirigami.Theme.fixedWidthFont
                background: null
            }
        }
    }

    SyntaxHighlighter {
        textEdit: area
        definition: loader.text.length > root.highlightLimit ? Repository.definitionForName("None") : Repository.definitionForFileName(PeekController.fileName)
        theme: Repository.defaultTheme(Kirigami.ColorUtils.brightnessForColor(Kirigami.Theme.backgroundColor) === Kirigami.ColorUtils.Dark ? Repository.DarkTheme : Repository.LightTheme)
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: loader.failed
        icon.name: "dialog-error"
        text: i18nc("@info", "This file could not be read")
    }
}
