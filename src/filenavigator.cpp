// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include "filenavigator.h"

#include <QCollator>
#include <QDir>
#include <QFileInfo>

#include <algorithm>

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
    m_items = siblings(url);
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

bool FileNavigator::isEmpty() const
{
    return m_items.isEmpty();
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

QList<QUrl> FileNavigator::siblings(const QUrl &url)
{
    if (!url.isLocalFile()) {
        return {url};
    }

    const QFileInfo info(url.toLocalFile());
    const QDir dir = info.absoluteDir();
    QFileInfoList entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::System);

    QCollator collator;
    collator.setNumericMode(true);
    collator.setCaseSensitivity(Qt::CaseInsensitive);

    std::sort(entries.begin(), entries.end(), [&collator](const QFileInfo &a, const QFileInfo &b) {
        if (a.isDir() != b.isDir()) {
            return a.isDir();
        }
        return collator.compare(a.fileName(), b.fileName()) < 0;
    });

    QList<QUrl> result;
    result.reserve(entries.size());
    for (const QFileInfo &entry : std::as_const(entries)) {
        result.append(QUrl::fromLocalFile(entry.absoluteFilePath()));
    }
    return result;
}
