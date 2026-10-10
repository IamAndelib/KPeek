// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

import io.github.iamandelib.kpeek

PageView {
    document: PdfDocument {
        url: PeekController.url
    }
}
