// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#pragma once

#include <QCollator>
#include <QLocale>

/// Dolphin-like name order: numbers compare by value ("2" before "10"), case is ignored.
/// Names that collate equal ("A"/"a") need a tie-break on the raw text to keep sorting strict.
inline QCollator naturalCollator()
{
    // The C locale (e.g. LANG unset, or a Flatpak without that locale's data) collates by raw bytes:
    // no numeric mode, no case folding. English rules come with ICU itself, so they always work.
    QLocale locale;
    if (locale.language() == QLocale::C) {
        locale = QLocale(QLocale::English);
    }
    QCollator collator(locale);
    collator.setNumericMode(true);
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    return collator;
}

/// Natural order for sorting, made strict by falling back to the raw text for names that collate equal.
inline bool naturalLess(const QCollator &collator, const QString &a, const QString &b)
{
    const int order = collator.compare(a, b);
    return order != 0 ? order < 0 : a < b;
}
