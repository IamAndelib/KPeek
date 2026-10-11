// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

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
    /// False for text too big to syntax-highlight without stalling the UI.
    Q_PROPERTY(bool highlight READ highlight NOTIFY loaded)
    Q_PROPERTY(qint64 maxBytes READ maxBytes CONSTANT)

public:
    using QObject::QObject;

    static constexpr qint64 MaxBytes = 2 * 1024 * 1024;
    static constexpr qint64 MaxHighlightChars = 512 * 1024;
    /// Longer lines are broken up: Qt lays out a line as one unit, and a 2 MB minified file on a
    /// single line takes several GB to show (about 350 MB as 10,000-character lines).
    static constexpr qsizetype MaxLineChars = 10000;

    QUrl url() const;
    void setUrl(const QUrl &url);

    QString text() const;
    bool truncated() const;
    bool failed() const;
    bool highlight() const;
    qint64 maxBytes() const;

    /// Decodes using a BOM if present, else UTF-8, falling back to Latin-1 for legacy 8-bit files.
    /// @p complete is false when @p data was cut off, so a character split at the end isn't an error.
    static QString decode(const QByteArray &data, bool complete = true);

    /// Inserts a line break after every MaxLineChars characters without one.
    static QString breakLongLines(const QString &text);

    static constexpr qint64 MaxMarkdownImageBytes = 16 * 1024 * 1024;
    /**
     * Markdown as HTML that is safe to show. Qt's text engine reads every image a document names, whole and
     * on the GUI thread: ![](/dev/zero) never ends. Only images that are regular local files up to
     * MaxMarkdownImageBytes are kept (as absolute paths, at their own size; @p base resolves relative ones),
     * and nothing else is loaded: no stylesheets, no background images.
     */
    Q_INVOKABLE static QString markdownToHtml(const QString &markdown, const QUrl &base);

Q_SIGNALS:
    void urlChanged();
    void loaded();

private:
    QUrl m_url;
    QString m_text;
    bool m_truncated = false;
    bool m_failed = false;
};
