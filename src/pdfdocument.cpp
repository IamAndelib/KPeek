// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include "pdfdocument.h"
#include "workerpool.h"

#include <poppler-qt6.h>

#include <QCache>
#include <QFileInfo>
#include <QMutex>
#include <QQuickTextureFactory>
#include <QRunnable>
#include <QtConcurrentRun>

#include <atomic>
#include <cmath>
#include <memory>

namespace
{
// Poppler documents aren't safe to use from several threads at once, so everything goes
// through one lock; recently used documents stay open to make scrolling cheap.
QMutex s_mutex;
QCache<QString, Poppler::Document> s_documents(4);

// Upper bound for one rendered page (~256 MB as ARGB32), however far the user zooms in.
constexpr double MaxPagePixels = 64e6;

QString pathFromKey(const QString &key)
{
    // Everything after the first slash: Qt may hand the id over already percent-decoded.
    return QUrl::fromPercentEncoding(key.section(QLatin1Char('/'), 1, -1).toUtf8());
}

/// The cached document for @p key. Call with s_mutex held. Sets @p locked for password-protected files.
Poppler::Document *documentLocked(const QString &key, bool *locked = nullptr)
{
    if (auto *document = s_documents.object(key)) {
        return document;
    }
    std::unique_ptr<Poppler::Document> document = Poppler::Document::load(pathFromKey(key));
    if (!document) {
        return nullptr;
    }
    if (document->isLocked()) {
        if (locked) {
            *locked = true;
        }
        return nullptr;
    }
    document->setRenderHint(Poppler::Document::Antialiasing);
    document->setRenderHint(Poppler::Document::TextAntialiasing);
    document->setRenderHint(Poppler::Document::TextHinting);
    auto *raw = document.release();
    s_documents.insert(key, raw);
    return raw;
}

class PageResponse : public QQuickImageResponse, public QRunnable
{
public:
    PageResponse(const QString &key, int page, const QSize &requestedSize)
        : m_key(key)
        , m_page(page)
        , m_requestedSize(requestedSize)
    {
        setAutoDelete(false);
    }

    void run() override
    {
        render();
        Q_EMIT finished();
    }

    void cancel() override
    {
        // Pages scrolled past or re-requested at a new zoom level are skipped instead of rendered.
        m_cancelled = true;
    }

    QQuickTextureFactory *textureFactory() const override
    {
        return QQuickTextureFactory::textureFactoryForImage(m_image);
    }

private:
    void render()
    {
        QMutexLocker locker(&s_mutex);
        if (m_cancelled) {
            return;
        }
        Poppler::Document *document = documentLocked(m_key);
        const std::unique_ptr<Poppler::Page> page = document ? document->page(m_page) : nullptr;
        if (!page) {
            return;
        }
        const QSizeF points = page->pageSizeF();
        if (points.isEmpty()) {
            return;
        }
        const int targetWidth = m_requestedSize.width() > 0 ? m_requestedSize.width() : 1024;
        const double maxDpi = std::sqrt(MaxPagePixels / (points.width() * points.height())) * 72.0;
        const double dpi = std::min(targetWidth / points.width() * 72.0, maxDpi);
        m_image = page->renderToImage(dpi, dpi);
    }

    QString m_key;
    int m_page;
    QSize m_requestedSize;
    QImage m_image;
    std::atomic_bool m_cancelled = false;
};
}

PdfDocument::PdfDocument(QObject *parent)
    : QObject(parent)
{
    connect(&m_watcher, &QFutureWatcher<Layout>::finished, this, [this] {
        m_layout = m_watcher.result();
        Q_EMIT loaded();
    });
}

PdfDocument::Layout PdfDocument::readLayout(const QString &key)
{
    Layout layout;
    QMutexLocker locker(&s_mutex);
    Poppler::Document *document = documentLocked(key, &layout.locked);
    if (!document) {
        layout.failed = !layout.locked;
        return layout;
    }
    const int count = document->numPages();
    layout.pageSizes.reserve(count);
    for (int i = 0; i < count; ++i) {
        const std::unique_ptr<Poppler::Page> page = document->page(i);
        layout.pageSizes.append(page ? page->pageSizeF() : QSizeF(595, 842));
    }
    return layout;
}

QUrl PdfDocument::url() const
{
    return m_url;
}

void PdfDocument::setUrl(const QUrl &url)
{
    if (m_url == url) {
        return;
    }
    m_url = url;
    Q_EMIT urlChanged();

    const QString path = url.toLocalFile();
    m_key = QString::number(QFileInfo(path).lastModified().toMSecsSinceEpoch()) + QLatin1Char('/') + QString::fromLatin1(QUrl::toPercentEncoding(path));
    m_layout = {};
    Q_EMIT loaded();
    m_watcher.setFuture(QtConcurrent::run(workerPool(), &PdfDocument::readLayout, m_key));
}

int PdfDocument::pageCount() const
{
    return m_layout.pageSizes.size();
}

bool PdfDocument::failed() const
{
    return m_layout.failed;
}

bool PdfDocument::locked() const
{
    return m_layout.locked;
}

QSizeF PdfDocument::pageSize(int page) const
{
    return m_layout.pageSizes.value(page, QSizeF(595, 842));
}

QString PdfDocument::pageSource(int page) const
{
    return QStringLiteral("image://%1/%2/%3").arg(QLatin1String(PdfPageProvider::Id)).arg(page).arg(m_key);
}

QQuickImageResponse *PdfPageProvider::requestImageResponse(const QString &id, const QSize &requestedSize)
{
    // id is "<page>/<mtime>/<percent-encoded path>"
    const qsizetype slash = id.indexOf(QLatin1Char('/'));
    auto *response = new PageResponse(id.mid(slash + 1), id.left(slash).toInt(), requestedSize);
    workerPool()->start(response);
    return response;
}

#include "moc_pdfdocument.cpp"
