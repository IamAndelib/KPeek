// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#pragma once

#include <QThreadPool>

/**
 * Pool for background work (PDF pages, archive listings, LibreOffice detection).
 *
 * Unlike QThreadPool::globalInstance(), the application doesn't wait for it on exit, so
 * quitting can't hang on a long uninterruptible job such as decompressing a huge tarball.
 * main() gives running jobs a short grace period and then exits anyway.
 */
inline QThreadPool *workerPool()
{
    static auto *pool = new QThreadPool; // intentionally leaked; see above
    return pool;
}
