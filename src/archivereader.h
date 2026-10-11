// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#pragma once

#include <QFile>
#include <QString>

#include <archive.h>
#include <archive_entry.h>

#include <memory>

// Shared libarchive helpers for archive listings and comic books.

using ArchivePtr = std::unique_ptr<archive, decltype(&archive_read_free)>;

/// Opens @p path for reading with every format, or returns null.
inline ArchivePtr openArchive(const QString &path)
{
    ArchivePtr reader(archive_read_new(), archive_read_free);
    archive_read_support_format_all(reader.get());
    // Built-in decompressors only. support_filter_all() also falls back to running external
    // programs (lrzip, lzop, grzip…) on the file, which an untrusted archive must never trigger.
    archive_read_support_filter_bzip2(reader.get());
    archive_read_support_filter_compress(reader.get());
    archive_read_support_filter_gzip(reader.get());
    archive_read_support_filter_lz4(reader.get());
    archive_read_support_filter_lzip(reader.get());
    archive_read_support_filter_lzma(reader.get());
    archive_read_support_filter_rpm(reader.get());
    archive_read_support_filter_uu(reader.get());
    archive_read_support_filter_xz(reader.get());
    archive_read_support_filter_zstd(reader.get());
    if (archive_read_open_filename(reader.get(), QFile::encodeName(path).constData(), 64 * 1024) != ARCHIVE_OK) {
        return ArchivePtr(nullptr, archive_read_free);
    }
    return reader;
}

/// The entry's path, without the leading "./" that archives made with "tar -C dir ." have.
inline QString entryPath(archive_entry *entry)
{
    // UTF-8 conversion fails for some names in the C locale (e.g. a Flatpak without the user's
    // locale); 7z and zip store names as Unicode, which the wide form still has.
    QString path;
    if (const char *utf8 = archive_entry_pathname_utf8(entry)) {
        path = QString::fromUtf8(utf8);
    } else if (const wchar_t *wide = archive_entry_pathname_w(entry)) {
        path = QString::fromWCharArray(wide);
    } else {
        path = QFile::decodeName(archive_entry_pathname(entry));
    }
    while (path.startsWith(QLatin1String("./"))) {
        path.remove(0, 2);
    }
    return path;
}

/// The next entry's header. libarchive reports recoverable problems (e.g. a name it can't convert) as warnings: keep going.
inline bool nextHeader(archive *reader, archive_entry **entry)
{
    const int result = archive_read_next_header(reader, entry);
    return result == ARCHIVE_OK || result == ARCHIVE_WARN;
}
