// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include "comicbook.h"
#include "naturalsort.h"
#include "systeminfo.h"
#include "workerpool.h"

#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QQuickTextureFactory>
#include <QRunnable>
#include <QSet>
#include <QtConcurrentRun>

#include <archive.h>
#include <archive_entry.h>

#include <algorithm>
#include <cmath>
#include <numeric>

namespace
{
using ArchivePtr = std::unique_ptr<archive, decltype(&archive_read_free)>;

// Enough of a page to read its dimensions; JPEG headers can sit behind a large EXIF block.
constexpr qsizetype HeaderBytes = 256 * 1024;
// A single page beyond this is treated as broken rather than extracted into memory.
constexpr qint64 MaxPageBytes = 256 * 1024 * 1024;

ArchivePtr openArchive(const QString &path)
{
    ArchivePtr reader(archive_read_new(), archive_read_free);
    archive_read_support_filter_all(reader.get());
    archive_read_support_format_all(reader.get());
    if (archive_read_open_filename(reader.get(), QFile::encodeName(path).constData(), 64 * 1024) != ARCHIVE_OK) {
        return ArchivePtr(nullptr, archive_read_free);
    }
    return reader;
}

QString entryPath(archive_entry *entry)
{
    const char *utf8 = archive_entry_pathname_utf8(entry);
    QString path = utf8 ? QString::fromUtf8(utf8) : QFile::decodeName(archive_entry_pathname(entry));
    // Archives made with "tar -C dir ." name their entries "./page1.jpg".
    while (path.startsWith(QLatin1String("./"))) {
        path.remove(0, 2);
    }
    return path;
}

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

/// Reads up to @p limit bytes of the current entry's data.
QByteArray readData(archive *reader, qint64 limit)
{
    QByteArray data;
    char buffer[64 * 1024];
    while (data.size() < limit) {
        const la_ssize_t read = archive_read_data(reader, buffer, sizeof(buffer));
        if (read <= 0) {
            break;
        }
        data.append(buffer, read);
    }
    return data;
}

class PageResponse : public QQuickImageResponse, public QRunnable
{
public:
    PageResponse(const QString &path, const QString &entry, int width)
        : m_path(path)
        , m_entry(entry)
        , m_width(width)
    {
        setAutoDelete(false);
    }

    void run() override
    {
        if (!m_cancelled) {
            m_image = ComicBook::readPage(m_path, m_entry, m_width);
        }
        Q_EMIT finished();
    }

    void cancel() override
    {
        m_cancelled = true; // scrolled past, or re-requested at another zoom level
    }

    QQuickTextureFactory *textureFactory() const override
    {
        return QQuickTextureFactory::textureFactoryForImage(m_image);
    }

private:
    QString m_path;
    QString m_entry;
    int m_width;
    QImage m_image;
    std::atomic_bool m_cancelled = false;
};
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

    archive_entry *entry = nullptr;
    while (archive_read_next_header(reader.get(), &entry) == ARCHIVE_OK) {
        if (cancelled && *cancelled) {
            return {};
        }
        const QString name = entryPath(entry);
        if (archive_entry_filetype(entry) != AE_IFREG || !isPage(name)) {
            continue;
        }
        QByteArray header = readData(reader.get(), HeaderBytes);
        QBuffer buffer(&header);
        QImageReader imageReader(&buffer);
        layout.entries.append(name);
        layout.sizes.append(QSizeF(imageReader.size()));
    }
    if (layout.entries.isEmpty()) {
        layout.failed = true;
        return layout;
    }

    // Pages in natural order ("page2" before "page10"), ties broken by the raw name.
    const QCollator collator = naturalCollator();
    QList<qsizetype> order(layout.entries.size());
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&](qsizetype a, qsizetype b) {
        const int c = collator.compare(layout.entries.at(a), layout.entries.at(b));
        return c != 0 ? c < 0 : layout.entries.at(a) < layout.entries.at(b);
    });
    Layout sorted;
    QSizeF previous(2, 3); // a typical page shape, for pages whose header didn't tell
    for (qsizetype i : std::as_const(order)) {
        const QSizeF size = layout.sizes.at(i).isEmpty() ? previous : layout.sizes.at(i);
        sorted.entries.append(layout.entries.at(i));
        sorted.sizes.append(size);
        previous = size;
    }
    return sorted;
}

QImage ComicBook::readPage(const QString &path, const QString &entryName, int width)
{
    const ArchivePtr reader = openArchive(path);
    if (!reader) {
        return {};
    }
    archive_entry *entry = nullptr;
    while (archive_read_next_header(reader.get(), &entry) == ARCHIVE_OK) {
        if (entryPath(entry) != entryName) {
            continue;
        }
        if (archive_entry_size(entry) > MaxPageBytes) {
            return {};
        }
        QByteArray data = readData(reader.get(), MaxPageBytes);
        QBuffer buffer(&data);
        QImageReader imageReader(&buffer);
        imageReader.setAutoTransform(true);
        const QSize size = imageReader.size();
        if (size.isValid() && width > 0) {
            // Decode at the shown width (never larger than the page), within the same pixel budget as PDF pages.
            const double maxPixels = isLowMemorySystem() ? 24e6 : 64e6;
            const double scale = std::min({1.0, double(width) / size.width(), std::sqrt(maxPixels / (double(size.width()) * size.height()))});
            imageReader.setScaledSize((QSizeF(size) * scale).toSize());
        }
        return imageReader.read();
    }
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

QSizeF ComicBook::pageSize(int page) const
{
    return m_layout.sizes.value(page, QSizeF(2, 3));
}

QString ComicBook::pageSource(int page) const
{
    // "<mtime>/<archive>/<entry>", both paths fully percent-encoded so neither contains a raw "/".
    return QStringLiteral("image://%1/%2/%3/%4")
        .arg(QLatin1String(ComicPageProvider::Id),
             QString::number(m_mtime),
             QString::fromLatin1(QUrl::toPercentEncoding(m_url.toLocalFile())),
             QString::fromLatin1(QUrl::toPercentEncoding(m_layout.entries.value(page))));
}

QQuickImageResponse *ComicPageProvider::requestImageResponse(const QString &id, const QSize &requestedSize)
{
    // Qt may hand the id over partly percent-decoded, but it keeps "%2F", so splitting on "/" is safe.
    const QStringList parts = id.split(QLatin1Char('/'));
    auto *response =
        new PageResponse(QUrl::fromPercentEncoding(parts.value(1).toUtf8()), QUrl::fromPercentEncoding(parts.value(2).toUtf8()), requestedSize.width());
    workerPool()->start(response);
    return response;
}

#include "moc_comicbook.cpp"
