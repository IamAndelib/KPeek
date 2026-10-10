// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#pragma once

#include <QDateTime>
#include <QList>
#include <QUrl>

/**
 * The list of files the user can step through and the position in it.
 *
 * A multi-file selection is browsed as-is, like PowerToys Peek. A single file is
 * browsed together with its siblings in the parent folder, sorted like Dolphin's
 * default view: folders first, then natural, case-insensitive name order.
 */
class FileNavigator
{
public:
    void setSelection(const QList<QUrl> &urls);

    QList<QUrl> items() const;
    int currentIndex() const;
    int count() const;
    QUrl current() const;
    bool isEmpty() const;

    /// Move by @p step, wrapping around at both ends.
    void step(int step);

    /// True when @p urls would reopen exactly what is shown now (used for Space toggling).
    bool matchesSelection(const QList<QUrl> &urls) const;

    static QList<QUrl> siblings(const QUrl &url);

private:
    /// siblings(), reusing the previous listing while the folder is unchanged:
    /// every key press in Dolphin re-invokes KPeek, and sorting a 100k-file folder isn't free.
    QList<QUrl> cachedSiblings(const QUrl &url);

    QString m_cachedDir;
    QDateTime m_cachedDirModified;
    QList<QUrl> m_cachedSiblings;

    QList<QUrl> m_selection;
    QList<QUrl> m_items;
    int m_index = -1;
};
