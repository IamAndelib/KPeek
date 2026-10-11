// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include "filenavigator.h"
#include "naturalsort.h"
#include "systeminfo.h"

#include <QCollator>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QStorageInfo>

#include <algorithm>
#include <vector>

void FileNavigator::setSelection(const QList<QUrl> &urls)
{
    m_selection = m_items = urls;
    m_index = urls.isEmpty() ? -1 : 0;
    m_siblingsNeeded = false;
    // One file: its whole folder.
    if (urls.size() != 1 || !urls.first().isLocalFile()) {
        return;
    }
    const QFileInfo dir(QFileInfo(urls.first().toLocalFile()).absolutePath());
    if (dir.absoluteFilePath() == m_cache.dir && dir.lastModified() == m_cache.dirModified) {
        setSiblings(m_cache);
    } else {
        m_siblingsNeeded = true;
    }
}

bool FileNavigator::siblingsNeeded() const
{
    return m_siblingsNeeded;
}

void FileNavigator::setSiblings(const Siblings &siblings)
{
    m_cache = siblings;
    m_siblingsNeeded = false;
    if (m_selection.size() != 1) {
        return;
    }
    // Hidden or unlistable files aren't in the listing: they are still previewed, just alone.
    const int index = siblings.urls.indexOf(m_selection.first());
    if (index >= 0) {
        m_items = siblings.urls;
        m_index = index;
    }
}

int FileNavigator::currentIndex() const
{
    return m_index;
}

int FileNavigator::count() const
{
    return m_items.size();
}

QUrl FileNavigator::current() const
{
    return m_index >= 0 ? m_items.at(m_index) : QUrl();
}

void FileNavigator::step(int step)
{
    if (m_items.isEmpty()) {
        return;
    }
    const int n = m_items.size();
    m_index = ((m_index + step) % n + n) % n;
}

bool FileNavigator::matchesSelection(const QList<QUrl> &urls) const
{
    if (m_items.isEmpty()) {
        return false;
    }
    if (urls.size() == 1) {
        return urls.first() == current();
    }
    return urls == m_selection;
}

FileNavigator::Siblings FileNavigator::listSiblings(const QUrl &url)
{
    const QFileInfo dir(QFileInfo(url.toLocalFile()).absolutePath());
    // The folder's time is taken first: a file added during the listing then makes it stale.
    return {dir.absoluteFilePath(), dir.lastModified(), siblings(url)};
}

QList<QUrl> FileNavigator::siblings(const QUrl &url)
{
    if (!url.isLocalFile()) {
        return {url};
    }

    const QDir dir = QFileInfo(url.toLocalFile()).absoluteDir();
    // NoSort: we sort ourselves below; QDir's own name sort would just be thrown away.
    const QFileInfoList entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::System, QDir::NoSort);

    const QCollator collator = naturalCollator();

    // Collation keys are computed once per file; comparing them is cheap even for 100k files.
    struct Item {
        bool isDir;
        QCollatorSortKey key;
        QString name;
        QString path;
    };
    std::vector<Item> items;
    items.reserve(entries.size());
    for (const QFileInfo &entry : entries) {
        items.push_back({entry.isDir(), collator.sortKey(entry.fileName()), entry.fileName(), entry.absoluteFilePath()});
    }

    std::sort(items.begin(), items.end(), [](const Item &a, const Item &b) {
        if (a.isDir != b.isDir) {
            return a.isDir;
        }
        const int order = a.key.compare(b.key);
        return order != 0 ? order < 0 : a.name < b.name; // "A" vs "a": keep the order strict
    });

    QList<QUrl> result;
    result.reserve(items.size());
    for (const Item &item : items) {
        result.append(QUrl::fromLocalFile(item.path));
    }
    return result;
}

QString homeTrash()
{
    if (isFlatpak()) {
        const QString hostData = qEnvironmentVariable("HOST_XDG_DATA_HOME");
        return (hostData.isEmpty() ? QDir::homePath() + QStringLiteral("/.local/share") : hostData) + QStringLiteral("/Trash");
    }
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/Trash");
}

QUrl trashedFileUrl(const QUrl &url, const QString &homeTrash)
{
    if (url.scheme() != QLatin1String("trash")) {
        return url;
    }
    // trash:/<trash number>-<name in its files folder>[/path inside a trashed folder]
    const QString path = url.path().mid(1);
    const qsizetype dash = path.indexOf(QLatin1Char('-'));
    if (dash <= 0 || path.contains(QLatin1String("/../")) || path.endsWith(QLatin1String("/.."))) {
        return url;
    }
    const QString entry = path.mid(dash + 1);
    if (entry.isEmpty() || entry.startsWith(QLatin1String(".."))) {
        return url;
    }
    QStringList trashes;
    if (path.first(dash) == QLatin1String("0")) {
        trashes = {homeTrash};
    } else {
        const QString uid = QString::number(getuid());
        const QList<QStorageInfo> volumes = QStorageInfo::mountedVolumes();
        for (const QStorageInfo &volume : volumes) {
            trashes << volume.rootPath() + QStringLiteral("/.Trash-") + uid << volume.rootPath() + QStringLiteral("/.Trash/") + uid;
        }
    }
    for (const QString &trash : std::as_const(trashes)) {
        const QString file = trash + QStringLiteral("/files/") + entry;
        if (QFileInfo::exists(file)) {
            return QUrl::fromLocalFile(file);
        }
    }
    return url;
}
