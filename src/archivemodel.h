// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#pragma once

#include <QAbstractListModel>
#include <QFutureWatcher>
#include <QQmlEngine>
#include <QUrl>

#include <atomic>
#include <memory>

struct ArchiveEntry {
    QString path;
    QString iconName;
    QString sizeText;
    bool isDir = false;
};

struct ArchiveListing {
    QList<ArchiveEntry> entries;
    int fileCount = 0;
    qint64 totalSize = 0;
    bool failed = false;
    bool truncated = false;
    bool incomplete = false; ///< the archive is damaged or cut off; entries up to that point are listed
};

/// Flat listing of an archive's contents (zip, tar.*, 7z, ar), read on the worker pool.
class ArchiveModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QUrl url READ url WRITE setUrl NOTIFY urlChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY statusChanged)
    Q_PROPERTY(bool failed READ failed NOTIFY statusChanged)
    Q_PROPERTY(bool truncated READ truncated NOTIFY statusChanged)
    Q_PROPERTY(bool incomplete READ incomplete NOTIFY statusChanged)
    Q_PROPERTY(int fileCount READ fileCount NOTIFY statusChanged)
    Q_PROPERTY(QString totalSizeText READ totalSizeText NOTIFY statusChanged)
    Q_PROPERTY(int maxEntries READ maxEntries CONSTANT)

public:
    enum Roles {
        PathRole = Qt::UserRole + 1,
        NameRole,
        DepthRole,
        SizeTextRole,
        IconNameRole,
    };

    static constexpr int MaxEntries = 20000;
    static constexpr qsizetype MaxPathChars = 4096;
    static constexpr qsizetype MaxDepth = 64;

    explicit ArchiveModel(QObject *parent = nullptr);
    ~ArchiveModel() override;

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QUrl url() const;
    void setUrl(const QUrl &url);

    bool loading() const;
    bool failed() const;
    bool truncated() const;
    bool incomplete() const;
    int fileCount() const;
    QString totalSizeText() const;
    int maxEntries() const;

    /// Reads the archive at @p path, stopping early once @p cancelled is set.
    /// Entries are sorted by path with folders before files, each folder directly above its contents.
    static ArchiveListing list(const QString &path, const std::atomic_bool *cancelled = nullptr);

Q_SIGNALS:
    void urlChanged();
    void statusChanged();

private:
    QUrl m_url;
    ArchiveListing m_listing;
    bool m_loading = false;
    std::shared_ptr<std::atomic_bool> m_cancelled;
    QFutureWatcher<ArchiveListing> m_watcher;
};
