// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.syntaxhighlighting

// Read-only, syntax-highlighted monospace text. Shared by the text viewer and Markdown's source view.
QQC2.TextArea {
    id: area

    required property string fileName
    property bool highlight: true
    // Wrapping a huge (often minified) file means laying out all of it again on every resize, so
    // by default only text small enough to highlight is wrapped.
    property bool wrap: highlight
    property real zoom: 1.0

    textFormat: TextEdit.PlainText
    readOnly: true
    selectByMouse: true
    wrapMode: wrap ? TextEdit.Wrap : TextEdit.NoWrap
    font.family: Kirigami.Theme.fixedWidthFont.family
    font.pointSize: Kirigami.Theme.fixedWidthFont.pointSize * zoom
    background: null

    // The highlighter lives and dies with this TextArea: KSyntaxHighlighting crashes if textEdit is ever set to null.
    SyntaxHighlighter {
        textEdit: area
        definition: area.highlight ? Repository.definitionForFileName(area.fileName) : Repository.definitionForName("None")
        theme: Repository.defaultTheme(Kirigami.ColorUtils.brightnessForColor(Kirigami.Theme.backgroundColor) === Kirigami.ColorUtils.Dark ? Repository.DarkTheme : Repository.LightTheme)
    }
}
