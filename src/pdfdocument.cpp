// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include "pdfdocument.h"
#include "imageresponse.h"
#include "systeminfo.h"
#include "workerpool.h"

#include <poppler-qt6.h>

#include <QCache>
#include <QDebug>
#include <QFileInfo>
#include <QMutex>
#include <QtConcurrentRun>

#include <atomic>
#include <cmath>
#include <memory>

namespace
{
// Poppler documents aren't safe to use from several threads at once, so everything goes
// through one lock; recently used documents stay open to make scrolling cheap.
QMutex s_mutex;
QCache<QString, Poppler::Document> s_documents(isLowMemorySystem() ? 2 : 4);

constexpr QSizeF A4Points(595, 842); // for pages whose size Poppler can't tell

/// The cached document for @p key. Call with s_mutex held. Sets @p locked for password-protected files.
Poppler::Document *openDocument(const QString &key, bool *locked = nullptr)
{
    const QString cacheKey = PdfDocument::cacheKey(key);
    if (auto *document = s_documents.object(cacheKey)) {
        return document;
    }
    std::unique_ptr<Poppler::Document> document = Poppler::Document::load(cacheKey.section(QLatin1Char('/'), 1));
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
    s_documents.insert(cacheKey, raw);
    return raw;
}

/// Renders @p pageNumber at about @p width pixels wide (within maxPagePixels()).
QImage renderPage(const QString &key, int pageNumber, int width, const std::atomic_bool &cancelled)
{
    QMutexLocker locker(&s_mutex);
    if (cancelled) {
        return {}; // queued behind other renders and scrolled past meanwhile
    }
    Poppler::Document *document = openDocument(key);
    const std::unique_ptr<Poppler::Page> page = document ? document->page(pageNumber) : nullptr;
    const QSizeF points = page ? page->pageSizeF() : QSizeF();
    if (points.isEmpty()) {
        qWarning() << "PDF page" << pageNumber << "of" << key << "could not be opened";
        return {};
    }
    const int targetWidth = width > 0 ? width : 1024;
    const double maxDpi = std::sqrt(maxPagePixels() / (points.width() * points.height())) * 72.0;
    const double dpi = std::min(targetWidth / points.width() * 72.0, maxDpi);
    QImage image = page->renderToImage(dpi, dpi);
    if (image.isNull()) {
        qWarning() << "PDF page" << pageNumber << "of" << key << "could not be rendered";
    }
    return image;
}
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
    Poppler::Document *document = openDocument(key, &layout.locked);
    if (!document) {
        layout.failed = !layout.locked;
        return layout;
    }
    const int count = document->numPages();
    layout.failed = count == 0; // Poppler "opens" some truncated files as documents without pages
    layout.pageSizes.reserve(count);
    for (int i = 0; i < count; ++i) {
        const std::unique_ptr<Poppler::Page> page = document->page(i);
        layout.pageSizes.append(page ? page->pageSizeF() : A4Points);
    }
    return layout;
}

QString PdfDocument::cacheKey(const QString &key)
{
    // key is "<mtime>/<path>". Qt hands image providers the id partly percent-decoded (spaces and
    // non-ASCII decoded, "%2F" kept), so normalise to the fully decoded path: otherwise the page
    // renderer and readLayout() would each open their own copy of the document.
    return key.section(QLatin1Char('/'), 0, 0) + QLatin1Char('/') + QUrl::fromPercentEncoding(key.section(QLatin1Char('/'), 1).toUtf8());
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
    return m_layout.pageSizes.value(page, A4Points);
}

QString PdfDocument::pageSource(int page) const
{
    return QStringLiteral("image://%1/%2/%3").arg(QLatin1String(PdfPageProvider::Id)).arg(page).arg(m_key);
}

QQuickImageResponse *PdfPageProvider::requestImageResponse(const QString &id, const QSize &requestedSize)
{
    // id is "<page>/<mtime>/<percent-encoded path>"
    const qsizetype slash = id.indexOf(QLatin1Char('/'));
    // "::" because the provider base class has an enum value named ImageResponse.
    auto *response =
        new ::ImageResponse([key = id.mid(slash + 1), page = id.left(slash).toInt(), width = requestedSize.width()](const std::atomic_bool &cancelled) {
            return renderPage(key, page, width, cancelled);
        });
    workerPool()->start(response);
    return response;
}

#include "moc_pdfdocument.cpp"
