// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include "archivemodel.h"
#include "archivereader.h"
#include "naturalsort.h"
#include "workerpool.h"

#include <KFormat>

#include <QCollator>
#include <QDebug>
#include <QMimeDatabase>
#include <QSet>
#include <QtConcurrentRun>

#include <algorithm>
#include <limits>
#include <numeric>

namespace
{
void sortListing(QList<ArchiveEntry> &entries)
{
    struct Key {
        QStringList segments;
        bool isDir;
    };
    QList<Key> keys;
    keys.reserve(entries.size());
    for (const ArchiveEntry &entry : std::as_const(entries)) {
        keys.append({entry.path.split(QLatin1Char('/')), entry.isDir});
    }

    const QCollator collator = naturalCollator();

    // Segment by segment, folders before files; names that collate equal ("A" vs "a",
    // "file01" vs "file1") are tie-broken by their raw text so the order is strict.
    const auto less = [&collator](const Key &a, const Key &b) {
        const qsizetype common = std::min(a.segments.size(), b.segments.size());
        for (qsizetype i = 0; i < common; ++i) {
            const QString &left = a.segments.at(i);
            const QString &right = b.segments.at(i);
            const bool leftIsDir = i < a.segments.size() - 1 || a.isDir;
            const bool rightIsDir = i < b.segments.size() - 1 || b.isDir;
            if (leftIsDir != rightIsDir) {
                return leftIsDir;
            }
            if (left == right) {
                continue;
            }
            return naturalLess(collator, left, right);
        }
        return a.segments.size() < b.segments.size();
    };

    QList<qsizetype> order(entries.size());
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&](qsizetype a, qsizetype b) {
        return less(keys.at(a), keys.at(b));
    });

    QList<ArchiveEntry> sorted;
    sorted.reserve(entries.size());
    for (qsizetype i : std::as_const(order)) {
        sorted.append(std::move(entries[i]));
    }
    entries = std::move(sorted);
}
}

ArchiveModel::ArchiveModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(&m_watcher, &QFutureWatcher<ArchiveListing>::finished, this, [this] {
        beginResetModel();
        m_listing = m_watcher.result();
        m_loading = false;
        endResetModel();
        Q_EMIT statusChanged();
    });
}

ArchiveModel::~ArchiveModel()
{
    if (m_cancelled) {
        *m_cancelled = true;
    }
}

ArchiveListing ArchiveModel::list(const QString &path, const std::atomic_bool *cancelled)
{
    ArchiveListing listing;
    // libarchive streams through the entry headers (skipping the data), so this needs no in-memory
    // tree and can stop between entries. It reads zip, 7z, rar, tar.*, iso, cpio, cab, ar (deb), rpm, …
    const ArchivePtr reader = openArchive(path);
    if (!reader) {
        listing.failed = true;
        return listing;
    }

    const KFormat format;
    QSet<QString> seen;
    // Adds an entry once; archives may repeat paths, and many zips have no entries for their folders.
    const auto add = [&](const QString &entryPath, bool isDir, qint64 size) {
        if (seen.contains(entryPath)) {
            return true;
        }
        if (listing.entries.size() >= MaxEntries) {
            listing.truncated = true;
            return false;
        }
        seen.insert(entryPath);
        ArchiveEntry item;
        item.path = entryPath;
        item.isDir = isDir;
        if (!isDir) {
            item.sizeText = format.formatByteSize(size);
            // Sizes come from the archive and can be absurd: saturate rather than overflow.
            listing.totalSize = size > 0 && listing.totalSize > std::numeric_limits<qint64>::max() - size ? std::numeric_limits<qint64>::max()
                                                                                                          : listing.totalSize + std::max<qint64>(size, 0);
            ++listing.fileCount;
        }
        listing.entries.append(item);
        return true;
    };

    archive_entry *entry = nullptr;
    int result = ARCHIVE_OK;
    while ((result = archive_read_next_header(reader.get(), &entry)) == ARCHIVE_OK || result == ARCHIVE_WARN) {
        if (cancelled && *cancelled) {
            return {};
        }
        QString entryPath = ::entryPath(entry);
        while (entryPath.endsWith(QLatin1Char('/'))) {
            entryPath.chop(1);
        }
        // Every parent folder becomes an entry, so a single crafted path thousands of levels deep
        // would cost gigabytes. Real archives stay far below these limits.
        if (entryPath.isEmpty() || entryPath == QLatin1String(".") || entryPath.size() > MaxPathChars || entryPath.count(QLatin1Char('/')) > MaxDepth) {
            archive_read_data_skip(reader.get());
            continue;
        }
        bool ok = true;
        for (qsizetype slash = entryPath.indexOf(QLatin1Char('/')); ok && slash > 0; slash = entryPath.indexOf(QLatin1Char('/'), slash + 1)) {
            ok = add(entryPath.left(slash), true, 0);
        }
        const bool isDir = archive_entry_filetype(entry) == AE_IFDIR;
        if (!ok || !add(entryPath, isDir, isDir ? 0 : archive_entry_size(entry))) {
            break;
        }
        archive_read_data_skip(reader.get());
    }
    if (result == ARCHIVE_FATAL) {
        qWarning() << "Archive" << path << "could not be read completely:" << archive_error_string(reader.get());
        if (listing.entries.isEmpty()) {
            listing.failed = true; // corrupt, encrypted headers, or not an archive after all
            return listing;
        }
        listing.incomplete = true; // e.g. a partial download: say so rather than look complete
    }

    QMimeDatabase mimeDatabase;
    for (ArchiveEntry &item : listing.entries) {
        item.iconName = item.isDir ? QStringLiteral("folder") : mimeDatabase.mimeTypeForFile(item.path, QMimeDatabase::MatchExtension).iconName();
    }
    sortListing(listing.entries);
    return listing;
}

int ArchiveModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_listing.entries.size();
}

QVariant ArchiveModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid)) {
        return {};
    }
    const ArchiveEntry &entry = m_listing.entries.at(index.row());
    switch (role) {
    case PathRole:
        return entry.path;
    case NameRole:
        return entry.path.section(QLatin1Char('/'), -1);
    case DepthRole:
        return entry.path.count(QLatin1Char('/'));
    case SizeTextRole:
        return entry.sizeText;
    case IconNameRole:
        return entry.iconName;
    }
    return {};
}

QHash<int, QByteArray> ArchiveModel::roleNames() const
{
    return {{PathRole, "path"}, {NameRole, "name"}, {DepthRole, "depth"}, {SizeTextRole, "sizeText"}, {IconNameRole, "iconName"}};
}

QUrl ArchiveModel::url() const
{
    return m_url;
}

void ArchiveModel::setUrl(const QUrl &url)
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

    beginResetModel();
    m_listing = {};
    m_loading = true;
    endResetModel();
    Q_EMIT statusChanged();

    m_watcher.setFuture(QtConcurrent::run(archivePool(), [path = url.toLocalFile(), cancelled = m_cancelled] {
        return list(path, cancelled.get());
    }));
}

bool ArchiveModel::loading() const
{
    return m_loading;
}

bool ArchiveModel::failed() const
{
    return m_listing.failed;
}

bool ArchiveModel::truncated() const
{
    return m_listing.truncated;
}

bool ArchiveModel::incomplete() const
{
    return m_listing.incomplete;
}

int ArchiveModel::fileCount() const
{
    return m_listing.fileCount;
}

QString ArchiveModel::totalSizeText() const
{
    return KFormat().formatByteSize(m_listing.totalSize);
}

int ArchiveModel::maxEntries() const
{
    return MaxEntries;
}

#include "moc_archivemodel.cpp"
