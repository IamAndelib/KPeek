// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibsriz@proton.me>

#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QUrl>

/// Reads a text file for the text and Markdown viewers, capped so huge logs stay responsive.
class TextLoader : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QUrl url READ url WRITE setUrl NOTIFY urlChanged)
    Q_PROPERTY(QString text READ text NOTIFY loaded)
    Q_PROPERTY(bool truncated READ truncated NOTIFY loaded)
    Q_PROPERTY(bool failed READ failed NOTIFY loaded)

public:
    using QObject::QObject;

    static constexpr qint64 MaxBytes = 2 * 1024 * 1024;

    QUrl url() const;
    void setUrl(const QUrl &url);

    QString text() const;
    bool truncated() const;
    bool failed() const;

    /// Decodes using a BOM if present, else UTF-8, falling back to the locale encoding.
    static QString decode(const QByteArray &data);

Q_SIGNALS:
    void urlChanged();
    void loaded();

private:
    QUrl m_url;
    QString m_text;
    bool m_truncated = false;
    bool m_failed = false;
};
