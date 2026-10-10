// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include "textloader.h"

#include <QDebug>
#include <QFile>
#include <QStringDecoder>

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
