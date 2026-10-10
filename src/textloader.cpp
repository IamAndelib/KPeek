// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibsriz@proton.me>

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
    m_text = m_failed ? QString() : decode(file.read(MaxBytes));
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

QString TextLoader::decode(const QByteArray &data)
{
    if (const auto bomEncoding = QStringConverter::encodingForData(data)) {
        QStringDecoder decoder(*bomEncoding, QStringDecoder::Flag::Stateless);
        return decoder.decode(data);
    }

    QStringDecoder utf8(QStringDecoder::Utf8);
    const QString text = utf8.decode(data);
    if (!utf8.hasError()) {
        return text;
    }
    QStringDecoder system(QStringDecoder::System);
    return system.decode(data);
}

#include "moc_textloader.cpp"
