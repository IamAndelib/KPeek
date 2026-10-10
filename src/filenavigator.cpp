// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include "filenavigator.h"
#include "naturalsort.h"

#include <QCollator>
#include <QDir>
#include <QFileInfo>

#include <algorithm>
#include <vector>

void FileNavigator::setSelection(const QList<QUrl> &urls)
{
    m_selection = urls;
    m_items.clear();
    m_index = -1;

    if (urls.isEmpty()) {
        return;
    }

    if (urls.size() > 1) {
        m_items = urls;
        m_index = 0;
        return;
    }

    const QUrl url = urls.first();
    m_items = cachedSiblings(url);
    m_index = m_items.indexOf(url);
    if (m_index < 0) {
        // Hidden or unlistable file: still preview it, just without siblings.
        m_items = {url};
        m_index = 0;
    }
}

QList<QUrl> FileNavigator::items() const
{
    return m_items;
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

QList<QUrl> FileNavigator::cachedSiblings(const QUrl &url)
{
    if (!url.isLocalFile()) {
        return {url};
    }
    const QFileInfo dir(QFileInfo(url.toLocalFile()).absolutePath());
    if (dir.absoluteFilePath() != m_cachedDir || dir.lastModified() != m_cachedDirModified) {
        m_cachedDir = dir.absoluteFilePath();
        m_cachedDirModified = dir.lastModified();
        m_cachedSiblings = siblings(url);
    }
    return m_cachedSiblings;
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
