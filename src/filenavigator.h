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
/**
 * The file behind a trash:/ URL from Dolphin's Trash, or @p url unchanged. trash:/0-name/… is in the home
 * trash, @p homeTrash (…/Trash); other numbers are the trash folders of other drives, which KIO numbers
 * at runtime, so those are found by name.
 */
QUrl trashedFileUrl(const QUrl &url, const QString &homeTrash);

/// The home trash folder (the host's, in the Flatpak).
QString homeTrash();

class FileNavigator
{
public:
    void setSelection(const QList<QUrl> &urls);

    int currentIndex() const;
    int count() const;
    QUrl current() const;

    /// Move by @p step, wrapping around at both ends.
    void step(int step);

    /// True when @p urls would reopen exactly what is shown now (so invoking KPeek again on it closes the preview).
    bool matchesSelection(const QList<QUrl> &urls) const;

    struct Siblings {
        QString dir;
        QDateTime dirModified;
        QList<QUrl> urls;
    };
    /// The files and folders next to @p url, sorted like Dolphin. Slow for huge folders: listing and
    /// sorting 100k entries takes about half a second, so this runs on the worker pool.
    static Siblings listSiblings(const QUrl &url);
    static QList<QUrl> siblings(const QUrl &url);

    /// After setSelection() of a single file whose folder isn't listed yet: it is browsed alone
    /// until setSiblings() delivers the listing.
    bool siblingsNeeded() const;
    /// Takes a listing (also kept for the next selection in that folder while it is unchanged:
    /// every key press in Dolphin re-invokes KPeek).
    void setSiblings(const Siblings &siblings);

private:
    Siblings m_cache;
    bool m_siblingsNeeded = false;

    QList<QUrl> m_selection;
    QList<QUrl> m_items;
    int m_index = -1;
};
