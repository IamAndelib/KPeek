// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#pragma once

#include <QtGlobal>

#include <clocale>
#include <langinfo.h>
#include <unistd.h>

/**
 * Whether this machine has little memory (4 GB or less). Such machines get lower caps for image
 * and PDF decoding, which is also where GPUs with small texture limits are found.
 * KPEEK_LOW_MEMORY=1 forces it, for testing or constrained setups.
 */
inline bool isLowMemorySystem()
{
    static const bool low = [] {
        if (qEnvironmentVariableIntValue("KPEEK_LOW_MEMORY") == 1) {
            return true;
        }
        const long pages = sysconf(_SC_PHYS_PAGES);
        const long pageSize = sysconf(_SC_PAGE_SIZE);
        // 4.5 GiB rather than 4: "4 GB" machines report a little less, and some a little more.
        return pages > 0 && pageSize > 0 && qint64(pages) * pageSize <= qint64(4608) * 1024 * 1024;
    }();
    return low;
}

/// Upper bound for one rendered PDF or comic page, however far the user zooms in:
/// ~256 MB as ARGB32 (~96 MB on low-memory machines).
inline double maxPagePixels()
{
    return isLowMemorySystem() ? 24e6 : 64e6;
}

/// Makes the C library's character set UTF-8 when it isn't, e.g. with LANG unset or in a Flatpak
/// without data for the user's locale. libarchive converts entry names through it, and in plain
/// "C" it drops every name that isn't ASCII. C.UTF-8 is built into glibc, so it always exists.
inline void ensureUtf8Locale()
{
    if (qstrcmp(nl_langinfo(CODESET), "UTF-8") != 0) {
        setlocale(LC_CTYPE, "C.UTF-8");
    }
}

/// Whether KPeek runs as a Flatpak.
inline bool isFlatpak()
{
    static const bool flatpak = access("/.flatpak-info", F_OK) == 0;
    return flatpak;
}
