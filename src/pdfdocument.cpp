// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include "pdfdocument.h"

#include <poppler-qt6.h>

#include <QCache>
#include <QMutex>
#include <QQuickTextureFactory>
#include <QRunnable>
#include <QThreadPool>

#include <memory>

namespace
{
// Poppler documents aren't safe to render from several threads at once, so all rendering
// goes through one lock; recently used documents stay open to make scrolling cheap.
QMutex s_documentsMutex;
QCache<QString, Poppler::Document> s_documents(4);

Poppler::Document *documentLocked(const QString &path)
{
    if (auto *document = s_documents.object(path)) {
        return document;
    }
    std::unique_ptr<Poppler::Document> document = Poppler::Document::load(path);
    if (!document || document->isLocked()) {
        return nullptr;
    }
    document->setRenderHint(Poppler::Document::Antialiasing);
    document->setRenderHint(Poppler::Document::TextAntialiasing);
    document->setRenderHint(Poppler::Document::TextHinting);
    auto *raw = document.release();
    s_documents.insert(path, raw);
    return raw;
}

class PageResponse : public QQuickImageResponse, public QRunnable
{
public:
    PageResponse(const QString &path, int page, const QSize &requestedSize)
        : m_path(path)
        , m_page(page)
        , m_requestedSize(requestedSize)
    {
        setAutoDelete(false);
    }

    void run() override
    {
        {
            QMutexLocker locker(&s_documentsMutex);
            if (auto *document = documentLocked(m_path)) {
                if (std::unique_ptr<Poppler::Page> page = document->page(m_page)) {
                    const QSizeF points = page->pageSizeF();
                    const int targetWidth = m_requestedSize.width() > 0 ? m_requestedSize.width() : 1024;
                    const double dpi = points.width() > 0 ? targetWidth / points.width() * 72.0 : 72.0;
                    m_image = page->renderToImage(dpi, dpi);
                }
            }
        }
        Q_EMIT finished();
    }

    QQuickTextureFactory *textureFactory() const override
    {
        return QQuickTextureFactory::textureFactoryForImage(m_image);
    }

private:
    QString m_path;
    int m_page;
    QSize m_requestedSize;
    QImage m_image;
};
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

    m_pageSizes.clear();
    std::unique_ptr<Poppler::Document> document = Poppler::Document::load(url.toLocalFile());
    m_failed = !document;
    m_locked = document && document->isLocked();
    if (document && !m_locked) {
        const int count = document->numPages();
        m_pageSizes.reserve(count);
        for (int i = 0; i < count; ++i) {
            const std::unique_ptr<Poppler::Page> page = document->page(i);
            m_pageSizes.append(page ? page->pageSizeF() : QSizeF(595, 842));
        }
    }
    Q_EMIT loaded();
}

int PdfDocument::pageCount() const
{
    return m_pageSizes.size();
}

bool PdfDocument::failed() const
{
    return m_failed;
}

bool PdfDocument::locked() const
{
    return m_locked;
}

QSizeF PdfDocument::pageSize(int page) const
{
    return m_pageSizes.value(page, QSizeF(595, 842));
}

QString PdfDocument::pageSource(int page) const
{
    return QStringLiteral("image://%1/%2/%3")
        .arg(QLatin1String(PdfPageProvider::Id))
        .arg(page)
        .arg(QString::fromLatin1(QUrl::toPercentEncoding(m_url.toLocalFile())));
}

QQuickImageResponse *PdfPageProvider::requestImageResponse(const QString &id, const QSize &requestedSize)
{
    const qsizetype slash = id.indexOf(QLatin1Char('/'));
    const int page = id.left(slash).toInt();
    const QString path = QUrl::fromPercentEncoding(id.mid(slash + 1).toLatin1());

    auto *response = new PageResponse(path, page, requestedSize);
    QThreadPool::globalInstance()->start(response);
    return response;
}

#include "moc_pdfdocument.cpp"
