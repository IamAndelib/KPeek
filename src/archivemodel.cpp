// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include "archivemodel.h"

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

#include <memory>

namespace
{
std::unique_ptr<KArchive> archiveFor(const QString &path, const QMimeType &mime)
{
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

void collect(const KArchiveDirectory *dir, const QString &prefix, ArchiveListing &listing)
{
    const QStringList names = dir->entries();
    for (const QString &name : names) {
        if (listing.entries.size() >= ArchiveModel::MaxEntries) {
            listing.truncated = true;
            return;
        }
        const KArchiveEntry *entry = dir->entry(name);
        ArchiveEntry item;
        item.path = prefix + name;
        item.isDir = entry->isDirectory();
        item.modified = entry->date();
        if (entry->isFile()) {
            item.size = static_cast<const KArchiveFile *>(entry)->size();
        }
        listing.entries.append(item);
        if (item.isDir) {
            collect(static_cast<const KArchiveDirectory *>(entry), item.path + QLatin1Char('/'), listing);
        }
    }
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

ArchiveListing ArchiveModel::list(const QString &path, const QString &mimeType)
{
    ArchiveListing listing;
    std::unique_ptr<KArchive> archive = archiveFor(path, QMimeDatabase().mimeTypeForName(mimeType));
    if (!archive->open(QIODevice::ReadOnly)) {
        listing.failed = true;
        return listing;
    }
    collect(archive->directory(), QString(), listing);

    QCollator collator;
    collator.setNumericMode(true);
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    // Comparing path segments keeps every folder directly above its own contents.
    std::sort(listing.entries.begin(), listing.entries.end(), [&collator](const ArchiveEntry &a, const ArchiveEntry &b) {
        const QStringList left = a.path.split(QLatin1Char('/'));
        const QStringList right = b.path.split(QLatin1Char('/'));
        for (qsizetype i = 0; i < qMin(left.size(), right.size()); ++i) {
            if (left[i] != right[i]) {
                const bool leftIsDir = i < left.size() - 1 || a.isDir;
                const bool rightIsDir = i < right.size() - 1 || b.isDir;
                if (leftIsDir != rightIsDir) {
                    return leftIsDir;
                }
                return collator.compare(left[i], right[i]) < 0;
            }
        }
        return left.size() < right.size();
    });
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
        return entry.isDir ? QString() : KFormat().formatByteSize(entry.size);
    case IsDirRole:
        return entry.isDir;
    case IconNameRole:
        return entry.isDir ? QStringLiteral("folder") : QMimeDatabase().mimeTypeForFile(entry.path, QMimeDatabase::MatchExtension).iconName();
    }
    return {};
}

QHash<int, QByteArray> ArchiveModel::roleNames() const
{
    return {{PathRole, "path"}, {NameRole, "name"}, {DepthRole, "depth"}, {SizeTextRole, "sizeText"}, {IsDirRole, "isDir"}, {IconNameRole, "iconName"}};
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

    beginResetModel();
    m_listing = {};
    m_loading = true;
    endResetModel();
    Q_EMIT statusChanged();

    const QString path = url.toLocalFile();
    const QString mimeType = QMimeDatabase().mimeTypeForFile(path).name();
    m_watcher.setFuture(QtConcurrent::run(&ArchiveModel::list, path, mimeType));
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
    return std::count_if(m_listing.entries.cbegin(), m_listing.entries.cend(), [](const ArchiveEntry &entry) {
        return !entry.isDir;
    });
}

QString ArchiveModel::totalSizeText() const
{
    qint64 total = 0;
    for (const ArchiveEntry &entry : m_listing.entries) {
        total += entry.size;
    }
    return KFormat().formatByteSize(total);
}

#include "moc_archivemodel.cpp"
