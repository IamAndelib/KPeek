// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include "textloader.h"

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStringDecoder>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>

QUrl TextLoader::url() const
{
    return m_url;
}

void TextLoader::setUrl(const QUrl &url)
{
    if (m_url == url) {
        return;
    }
    m_url = url;
    Q_EMIT urlChanged();

    QFile file(url.toLocalFile());
    m_failed = !file.open(QIODevice::ReadOnly);
    if (m_failed) {
        qWarning() << "Could not read" << file.fileName() << file.errorString();
    }
    m_truncated = !m_failed && file.size() > MaxBytes;
    m_text = m_failed ? QString() : breakLongLines(decode(file.read(MaxBytes), !m_truncated));
    Q_EMIT loaded();
}

QString TextLoader::text() const
{
    return m_text;
}

bool TextLoader::truncated() const
{
    return m_truncated;
}

bool TextLoader::failed() const
{
    return m_failed;
}

qint64 TextLoader::maxBytes() const
{
    return MaxBytes;
}

bool TextLoader::highlight() const
{
    return m_text.size() <= MaxHighlightChars;
}

QString TextLoader::decode(const QByteArray &data, bool complete)
{
    if (const auto bomEncoding = QStringConverter::encodingForData(data)) {
        QStringDecoder decoder(*bomEncoding, QStringDecoder::Flag::Stateless);
        return decoder.decode(data);
    }

    // Stateless: an unfinished character at the end of complete data is an error, not something to wait for.
    QStringDecoder utf8(QStringDecoder::Utf8, complete ? QStringDecoder::Flag::Stateless : QStringDecoder::Flag::Default);
    const QString text = utf8.decode(data);
    if (!utf8.hasError()) {
        return text;
    }
    // Not "System": that is UTF-8 on every modern system and would just repeat the failure.
    return QString::fromLatin1(data);
}

QString TextLoader::breakLongLines(const QString &text)
{
    QString result;
    qsizetype lineStart = 0;
    qsizetype copied = 0;
    for (qsizetype i = 0; i < text.size(); ++i) {
        if (text.at(i) == QLatin1Char('\n')) {
            lineStart = i + 1;
        } else if (i - lineStart == MaxLineChars) {
            if (result.isEmpty()) {
                result.reserve(text.size() + text.size() / MaxLineChars);
            }
            result.append(QStringView(text).sliced(copied, i - copied)).append(QLatin1Char('\n'));
            copied = lineStart = i;
        }
    }
    if (copied == 0) {
        return text; // the common case: nothing to break, no copy
    }
    return result.append(QStringView(text).sliced(copied));
}

#include "moc_textloader.cpp"

namespace
{
/// Parsing HTML loads stylesheets and background images right away: this document loads nothing.
class InertDocument : public QTextDocument
{
protected:
    QVariant loadResource(int, const QUrl &) override
    {
        return {};
    }
};
}

QString TextLoader::markdownToHtml(const QString &markdown, const QUrl &base)
{
    InertDocument document;
    document.setMarkdown(markdown, QTextDocument::MarkdownDialectGitHub);

    // Collected first, then changed from the end, so earlier positions stay valid.
    QList<std::pair<int, QString>> images; // position, absolute file URL or empty to remove
    for (QTextBlock block = document.begin(); block.isValid(); block = block.next()) {
        for (auto it = block.begin(); !it.atEnd(); ++it) {
            const QTextFragment fragment = it.fragment();
            if (!fragment.charFormat().isImageFormat()) {
                continue;
            }
            const QUrl url = base.resolved(QUrl(fragment.charFormat().toImageFormat().name()));
            const QFileInfo info(url.toLocalFile());
            const bool safe = url.isLocalFile() && info.isFile() && info.size() <= MaxMarkdownImageBytes;
            for (int i = 0; i < fragment.length(); ++i) {
                images.append({fragment.position() + i, safe ? QUrl::fromLocalFile(info.absoluteFilePath()).toString() : QString()});
            }
        }
    }
    for (auto it = images.crbegin(); it != images.crend(); ++it) {
        QTextCursor cursor(&document);
        cursor.setPosition(it->first);
        cursor.setPosition(it->first + 1, QTextCursor::KeepAnchor);
        if (it->second.isEmpty()) {
            cursor.removeSelectedText();
        } else {
            QTextImageFormat format = cursor.charFormat().toImageFormat();
            format.setName(it->second);
            format.clearProperty(QTextFormat::ImageWidth); // its own size: <img width="100000"> would scale it up
            format.clearProperty(QTextFormat::ImageHeight);
            cursor.setCharFormat(format);
        }
    }

    // The body's font would override the viewer's (and its zoom). Background images (HTML tables and cells
    // can name one) would be loaded by the viewer: none.
    static const QRegularExpression bodyStyle(QStringLiteral("<body style=\"[^\"]*\">"));
    static const QRegularExpression background(QStringLiteral(" background=\"[^\"]*\""));
    return document.toHtml().replace(bodyStyle, QStringLiteral("<body>")).remove(background);
}
