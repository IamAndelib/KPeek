// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#pragma once

#include <QtGlobal>

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
