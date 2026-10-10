// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#pragma once

#include <QFutureWatcher>
#include <QImage>
#include <QList>
#include <QObject>
#include <QQmlEngine>
#include <QQuickAsyncImageProvider>
#include <QSizeF>
#include <QUrl>

#include <atomic>
#include <memory>

/**
 * A comic book archive (CBZ, CBR, CB7, CBT) as a list of pages, with the same interface as
 * PdfDocument so PageView shows both. Pages are the archive's images in natural name order;
 * each is extracted and decoded only when shown, by ComicPageProvider ("image://kpeekcomic/…").
 */
class ComicBook : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QUrl url READ url WRITE setUrl NOTIFY urlChanged)
    Q_PROPERTY(int pageCount READ pageCount NOTIFY loaded)
    Q_PROPERTY(bool failed READ failed NOTIFY loaded)

public:
    explicit ComicBook(QObject *parent = nullptr);
    ~ComicBook() override;

    QUrl url() const;
    void setUrl(const QUrl &url);

    int pageCount() const;
    bool failed() const;

    /// Page size in pixels (only the aspect ratio matters to PageView).
    Q_INVOKABLE QSizeF pageSize(int page) const;
    /// Image provider source for @p page.
    Q_INVOKABLE QString pageSource(int page) const;

    struct Layout {
        QStringList entries; ///< archive paths of the pages, in reading order
        QList<QSizeF> sizes;
        bool failed = false;
    };
    /// One pass over the archive: finds the pages and reads their dimensions. Stops early once @p cancelled is set.
    static Layout readLayout(const QString &path, const std::atomic_bool *cancelled = nullptr);

    /// Extracts @p entry from the archive at @p path and decodes it at most @p width pixels wide.
    static QImage readPage(const QString &path, const QString &entry, int width);

Q_SIGNALS:
    void urlChanged();
    void loaded();

private:
    QUrl m_url;
    qint64 m_mtime = 0;
    Layout m_layout;
    std::shared_ptr<std::atomic_bool> m_cancelled;
    QFutureWatcher<Layout> m_watcher;
};

class ComicPageProvider : public QQuickAsyncImageProvider
{
public:
    static constexpr auto Id = "kpeekcomic";

    QQuickImageResponse *requestImageResponse(const QString &id, const QSize &requestedSize) override;
};
