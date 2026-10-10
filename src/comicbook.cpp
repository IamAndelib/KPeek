// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include "comicbook.h"
#include "archivereader.h"
#include "imageresponse.h"
#include "naturalsort.h"
#include "systeminfo.h"
#include "workerpool.h"

#include <QBuffer>
#include <QDebug>
#include <QFileInfo>
#include <QImageReader>
#include <QScopeGuard>
#include <QSemaphore>
#include <QSet>
#include <QtConcurrentRun>

#include <algorithm>
#include <cmath>

namespace
{
// Enough of a page to read its dimensions; JPEG headers can sit behind a large EXIF block.
constexpr qsizetype HeaderBytes = 256 * 1024;
// Bounds for crafted archives: more pages than any real book, and a page beyond this size is
// treated as broken rather than extracted into memory.
constexpr qsizetype MaxPages = 10000;
qint64 maxPageBytes()
{
    return isLowMemorySystem() ? 32 * 1024 * 1024 : 64 * 1024 * 1024;
}

// At most two pages are extracted at once, so a book of decompression bombs can't take a
// page-sized buffer per CPU core. Decoding a page takes milliseconds; scrolling doesn't notice.
QSemaphore s_pageSlots(2);

/// Images that are pages: not macOS resource forks or other hidden files.
bool isPage(const QString &path)
{
    static const QSet<QByteArray> formats = [] {
        const QList<QByteArray> list = QImageReader::supportedImageFormats();
        return QSet<QByteArray>(list.cbegin(), list.cend());
    }();
    if (path.startsWith(QLatin1String("__MACOSX/")) || path.contains(QLatin1String("/.")) || path.startsWith(QLatin1Char('.'))) {
        return false;
    }
    return formats.contains(QFileInfo(path).suffix().toLower().toLatin1());
}

/// Reads up to @p limit bytes of the current entry's data; null if it can't be extracted.
QByteArray readData(archive *reader, qint64 limit)
{
    QByteArray data;
    char buffer[64 * 1024];
    while (data.size() < limit) {
        const la_ssize_t read = archive_read_data(reader, buffer, sizeof(buffer));
        if (read < 0) {
            qWarning() << "Comic book page could not be extracted:" << archive_error_string(reader);
            return {};
        }
        if (read == 0) {
            break;
        }
        data.append(buffer, read);
    }
    return data;
}

/// libarchive reports recoverable problems (e.g. a name it can't convert) as warnings: keep going.
bool nextHeader(archive *reader, archive_entry **entry)
{
    const int result = archive_read_next_header(reader, entry);
    return result == ARCHIVE_OK || result == ARCHIVE_WARN;
}
}

ComicBook::ComicBook(QObject *parent)
    : QObject(parent)
{
    connect(&m_watcher, &QFutureWatcher<Layout>::finished, this, [this] {
        m_layout = m_watcher.result();
        Q_EMIT loaded();
    });
}

ComicBook::~ComicBook()
{
    if (m_cancelled) {
        *m_cancelled = true;
    }
}

ComicBook::Layout ComicBook::readLayout(const QString &path, const std::atomic_bool *cancelled)
{
    Layout layout;
    const ArchivePtr reader = openArchive(path);
    if (!reader) {
        layout.failed = true;
        return layout;
    }

    struct Page {
        QString entry;
        QSizeF size;
    };
    QList<Page> pages;
    archive_entry *entry = nullptr;
    while (nextHeader(reader.get(), &entry) && pages.size() < MaxPages) {
        if (cancelled && *cancelled) {
            return {};
        }
        const QString name = entryPath(entry);
        if (archive_entry_filetype(entry) != AE_IFREG || !isPage(name)) {
            continue;
        }
        if (archive_entry_is_encrypted(entry)) {
            layout.locked = true; // password-protected: PageView says so instead of showing blank pages
            return layout;
        }
        QByteArray header = readData(reader.get(), HeaderBytes);
        QBuffer buffer(&header);
        pages.append({name, QSizeF(QImageReader(&buffer).size())});
    }
    if (pages.isEmpty()) {
        layout.failed = true;
        return layout;
    }

    // Pages in natural order ("page2" before "page10"), ties broken by the raw name.
    const QCollator collator = naturalCollator();
    std::sort(pages.begin(), pages.end(), [&collator](const Page &a, const Page &b) {
        return naturalLess(collator, a.entry, b.entry);
    });
    QSizeF previous = FallbackPageShape; // for pages whose header didn't tell
    for (const Page &page : std::as_const(pages)) {
        previous = page.size.isEmpty() ? previous : page.size;
        layout.entries.append(page.entry);
        layout.sizes.append(previous);
    }
    return layout;
}

QImage ComicBook::readPage(const QString &path, const QString &entryName, int width, const std::atomic_bool *cancelled)
{
    s_pageSlots.acquire();
    const auto release = qScopeGuard([] {
        s_pageSlots.release();
    });
    const ArchivePtr reader = openArchive(path);
    archive_entry *entry = nullptr;
    while (reader && nextHeader(reader.get(), &entry)) {
        if (cancelled && *cancelled) {
            return {};
        }
        if (entryPath(entry) != entryName) {
            continue;
        }
        if (archive_entry_size(entry) > maxPageBytes()) {
            qWarning() << "Comic book page" << entryName << "is too large to show";
            return {};
        }
        QByteArray data = readData(reader.get(), maxPageBytes());
        QBuffer buffer(&data);
        QImageReader imageReader(&buffer);
        imageReader.setAutoTransform(true);
        const QSize size = imageReader.size();
        if (size.isValid() && width > 0) {
            // Decode at the shown width (never larger than the page), within the page pixel budget.
            const double scale = std::min({1.0, double(width) / size.width(), std::sqrt(maxPagePixels() / (double(size.width()) * size.height()))});
            imageReader.setScaledSize((QSizeF(size) * scale).toSize());
        }
        QImage image = imageReader.read();
        if (image.isNull()) {
            qWarning() << "Comic book page" << entryName << "could not be decoded:" << imageReader.errorString();
        }
        return image;
    }
    qWarning() << "Comic book page" << entryName << "was not found in" << path;
    return {};
}

QUrl ComicBook::url() const
{
    return m_url;
}

void ComicBook::setUrl(const QUrl &url)
{
    if (m_url == url) {
        return;
    }
    m_url = url;
    Q_EMIT urlChanged();

    if (m_cancelled) {
        *m_cancelled = true;
    }
    m_cancelled = std::make_shared<std::atomic_bool>(false);
    m_layout = {};
    Q_EMIT loaded();

    const QString path = url.toLocalFile();
    m_mtime = QFileInfo(path).lastModified().toMSecsSinceEpoch();
    m_watcher.setFuture(QtConcurrent::run(archivePool(), [path, cancelled = m_cancelled] {
        return readLayout(path, cancelled.get());
    }));
}

int ComicBook::pageCount() const
{
    return m_layout.entries.size();
}

bool ComicBook::failed() const
{
    return m_layout.failed;
}

bool ComicBook::locked() const
{
    return m_layout.locked;
}

QSizeF ComicBook::pageSize(int page) const
{
    return m_layout.sizes.value(page, FallbackPageShape);
}

QString ComicBook::pageSource(int page) const
{
    return ArchiveImageProvider::source(m_url.toLocalFile(), m_mtime, m_layout.entries.value(page));
}

QString ArchiveImageProvider::source(const QString &archivePath, qint64 mtime, const QString &entry)
{
    // "<mtime>/<archive>/<entry>", both paths fully percent-encoded so neither contains a raw "/".
    // The mtime makes an edited archive a new image.
    return QStringLiteral("image://%1/%2/%3/%4")
        .arg(QLatin1String(Id),
             QString::number(mtime),
             QString::fromLatin1(QUrl::toPercentEncoding(archivePath)),
             QString::fromLatin1(QUrl::toPercentEncoding(entry)));
}

QQuickImageResponse *ArchiveImageProvider::requestImageResponse(const QString &id, const QSize &requestedSize)
{
    // Qt may hand the id over partly percent-decoded, but it keeps "%2F", so splitting on "/" is safe.
    const QStringList parts = id.split(QLatin1Char('/'));
    // "::" because the provider base class has an enum value named ImageResponse.
    auto *response = new ::ImageResponse([path = QUrl::fromPercentEncoding(parts.value(1).toUtf8()),
                                          entry = QUrl::fromPercentEncoding(parts.value(2).toUtf8()),
                                          width = requestedSize.width()](const std::atomic_bool &cancelled) {
        return ComicBook::readPage(path, entry, width, &cancelled);
    });
    workerPool()->start(response);
    return response;
}

#include "moc_comicbook.cpp"
