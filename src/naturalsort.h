// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#pragma once

#include <QCollator>

/// Dolphin-like name order: numbers compare by value ("2" before "10"), case is ignored.
/// Names that collate equal ("A"/"a") need a tie-break on the raw text to keep sorting strict.
inline QCollator naturalCollator()
{
    QCollator collator;
    collator.setNumericMode(true);
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    return collator;
}
