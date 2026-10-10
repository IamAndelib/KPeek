// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include "archivemodel.h"
#include "naturalsort.h"
#include "workerpool.h"

#include <K7Zip>
#include <KAr>
#include <KArchiveDirectory>
#include <KArchiveFile>
#include <KFormat>
#include <KTar>
#include <KZip>

#include <QCollator>
#include <QMimeDatabase>
#include <QtConcurrentRun>

#include <algorithm>
#include <numeric>

namespace
{
std::unique_ptr<KArchive> archiveFor(const QString &path)
{
    const QMimeType mime = QMimeDatabase().mimeTypeForFile(path);
    if (mime.inherits(QStringLiteral("application/zip"))) {
        return std::make_unique<KZip>(path);
    }
    if (mime.inherits(QStringLiteral("application/x-7z-compressed"))) {
        return std::make_unique<K7Zip>(path);
    }
    if (mime.inherits(QStringLiteral("application/x-archive"))) {
        return std::make_unique<KAr>(path);
    }
    // KTar picks the decompression filter (gz, bz2, xz, zstd, lzma) from the file itself.
    return std::make_unique<KTar>(path);
}

void collect(const KArchiveDirectory *dir, const QString &prefix, ArchiveListing &listing, const std::atomic_bool *cancelled, const KFormat &format)
{
    const QStringList names = dir->entries();
    for (const QString &name : names) {
        if (cancelled && *cancelled) {
            return;
        }
        if (listing.entries.size() >= ArchiveModel::MaxEntries) {
            listing.truncated = true;
            return;
        }
        const KArchiveEntry *entry = dir->entry(name);
        ArchiveEntry item;
        item.path = prefix + name;
        item.isDir = entry->isDirectory();
        listing.entries.append(item);
        if (item.isDir) {
            collect(static_cast<const KArchiveDirectory *>(entry), item.path + QLatin1Char('/'), listing, cancelled, format);
        } else {
            const qint64 size = static_cast<const KArchiveFile *>(entry)->size();
            listing.entries.last().sizeText = format.formatByteSize(size);
            listing.totalSize += size;
            ++listing.fileCount;
        }
    }
}

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
            const int order = collator.compare(left, right);
            return order != 0 ? order < 0 : left < right;
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
    std::unique_ptr<KArchive> archive = archiveFor(path);
    if (!archive->open(QIODevice::ReadOnly)) {
        listing.failed = true;
        return listing;
    }
    collect(archive->directory(), QString(), listing, cancelled, KFormat());
    if (cancelled && *cancelled) {
        return {};
    }

    QMimeDatabase mimeDatabase;
    for (ArchiveEntry &entry : listing.entries) {
        entry.iconName = entry.isDir ? QStringLiteral("folder") : mimeDatabase.mimeTypeForFile(entry.path, QMimeDatabase::MatchExtension).iconName();
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
