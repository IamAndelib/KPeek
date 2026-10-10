// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include "textloader.h"

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
    m_truncated = !m_failed && file.size() > MaxBytes;
    m_text = m_failed ? QString() : decode(file.read(MaxBytes), !m_truncated);
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
    QStringDecoder latin1(QStringDecoder::Latin1);
    return latin1.decode(data);
}

#include "moc_textloader.cpp"
