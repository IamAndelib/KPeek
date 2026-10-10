// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QFutureWatcher>
#include <QQmlEngine>
#include <QUrl>

struct ArchiveEntry {
    QString path;
    qint64 size = 0;
    bool isDir = false;
    QDateTime modified;
};

struct ArchiveListing {
    QList<ArchiveEntry> entries;
    bool failed = false;
    bool truncated = false;
};

/// Flat listing of an archive's contents (zip, tar.*, 7z, ar), read in a worker thread.
class ArchiveModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QUrl url READ url WRITE setUrl NOTIFY urlChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY statusChanged)
    Q_PROPERTY(bool failed READ failed NOTIFY statusChanged)
    Q_PROPERTY(bool truncated READ truncated NOTIFY statusChanged)
    Q_PROPERTY(int fileCount READ fileCount NOTIFY statusChanged)
    Q_PROPERTY(QString totalSizeText READ totalSizeText NOTIFY statusChanged)
    Q_PROPERTY(int maxEntries MEMBER s_maxEntries CONSTANT)

public:
    enum Roles {
        PathRole = Qt::UserRole + 1,
        NameRole,
        DepthRole,
        SizeTextRole,
        IsDirRole,
        IconNameRole,
    };

    static constexpr int MaxEntries = 20000;
    static constexpr int s_maxEntries = MaxEntries; // for QML

    explicit ArchiveModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QUrl url() const;
    void setUrl(const QUrl &url);

    bool loading() const;
    bool failed() const;
    bool truncated() const;
    int fileCount() const;
    QString totalSizeText() const;

    /// Reads the archive at @p path; entries are sorted by path, folders before their contents.
    static ArchiveListing list(const QString &path, const QString &mimeType);

Q_SIGNALS:
    void urlChanged();
    void statusChanged();

private:
    QUrl m_url;
    ArchiveListing m_listing;
    bool m_loading = false;
    QFutureWatcher<ArchiveListing> m_watcher;
};
