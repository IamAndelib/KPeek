// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

import io.github.iamandelib.kpeek

// Comic books (CBZ, CBR, CB7, CBT): the archive's images as scrollable pages.
PageView {
    document: ComicBook {
        url: PeekController.url
    }
}
