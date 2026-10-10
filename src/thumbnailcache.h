// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#pragma once

#include <QString>
#include <QUrl>

/**
 * Lookup in the freedesktop thumbnail cache (~/.cache/thumbnails), which Dolphin fills.
 * Those thumbnails are free, instant low-resolution stand-ins ("proxies") shown while
 * the real image or video loads.
 */
namespace ThumbnailCache
{
/// The largest up-to-date thumbnail of @p path, or an empty URL if there is none.
QUrl find(const QString &path, const QString &cacheRoot = QString());

/// The freedesktop cache file name for @p path: MD5 of its file:// URI, plus ".png".
QString fileNameFor(const QString &path);
}
