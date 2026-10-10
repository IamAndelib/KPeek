// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include "viewerresolver.h"

#include <QFile>
#include <QImageReader>
#include <QStringDecoder>

namespace
{
bool inheritsAny(const QMimeType &mime, const QStringList &types)
{
    for (const QString &type : types) {
        if (mime.inherits(type)) {
            return true;
        }
    }
    return false;
}

bool isOffice(const QMimeType &mime)
{
    const QString name = mime.name();
    if (name.startsWith(QLatin1String("application/vnd.oasis.opendocument."))
        || name.startsWith(QLatin1String("application/vnd.openxmlformats-officedocument.")) || name.startsWith(QLatin1String("application/vnd.ms-excel"))
        || name.startsWith(QLatin1String("application/vnd.ms-powerpoint")) || name.startsWith(QLatin1String("application/vnd.ms-word"))) {
        return true;
    }
    return inheritsAny(mime,
                       {QStringLiteral("application/msword"),
                        QStringLiteral("application/rtf"),
                        QStringLiteral("application/vnd.visio"),
                        QStringLiteral("application/x-iwork-pages-sffpages"),
                        QStringLiteral("application/x-iwork-numbers-sffnumbers"),
                        QStringLiteral("application/x-iwork-keynote-sffkey")});
}

bool isModel(const QMimeType &mime)
{
    // What Qt Quick 3D's asset importer reads and shared-mime-info names (no FBX/PLY/Collada types exist).
    return inheritsAny(mime,
                       {QStringLiteral("model/gltf+json"),
                        QStringLiteral("model/gltf-binary"),
                        QStringLiteral("model/obj"),
                        QStringLiteral("model/stl"),
                        QStringLiteral("model/x.stl-ascii"),
                        QStringLiteral("model/x.stl-binary"),
                        QStringLiteral("model/3mf")});
}

bool isComic(const QMimeType &mime)
{
    return inheritsAny(mime,
                       {QStringLiteral("application/vnd.comicbook+zip"),
                        QStringLiteral("application/vnd.comicbook-rar"),
                        QStringLiteral("application/x-cb7"),
                        QStringLiteral("application/x-cbt")});
}

bool isArchive(const QMimeType &mime)
{
    return inheritsAny(mime,
                       {QStringLiteral("application/zip"),
                        QStringLiteral("application/x-tar"),
                        QStringLiteral("application/x-compressed-tar"),
                        QStringLiteral("application/x-bzip2-compressed-tar"),
                        QStringLiteral("application/x-bzip-compressed-tar"),
                        QStringLiteral("application/x-xz-compressed-tar"),
                        QStringLiteral("application/x-zstd-compressed-tar"),
                        QStringLiteral("application/x-lzma-compressed-tar"),
                        QStringLiteral("application/x-lz4-compressed-tar"),
                        QStringLiteral("application/x-lrzip-compressed-tar"),
                        QStringLiteral("application/x-tarz"),
                        QStringLiteral("application/x-7z-compressed"),
                        QStringLiteral("application/vnd.rar"),
                        QStringLiteral("application/x-cd-image"),
                        QStringLiteral("application/vnd.debian.binary-package"),
                        QStringLiteral("application/x-rpm"),
                        QStringLiteral("application/vnd.ms-cab-compressed"),
                        QStringLiteral("application/x-cpio"),
                        QStringLiteral("application/x-xar"),
                        QStringLiteral("application/x-lha"),
                        QStringLiteral("application/x-archive")});
}

bool isFont(const QMimeType &mime)
{
    return mime.name().startsWith(QLatin1String("font/"))
        || inheritsAny(mime,
                       {QStringLiteral("application/x-font-ttf"), QStringLiteral("application/x-font-otf"), QStringLiteral("application/vnd.ms-opentype")});
}

bool isText(const QMimeType &mime)
{
    // Many textual formats (JSON, XML, scripts, YAML...) don't inherit text/plain in shared-mime-info.
    return inheritsAny(mime,
                       {QStringLiteral("text/plain"),
                        QStringLiteral("application/json"),
                        QStringLiteral("application/xml"),
                        QStringLiteral("application/javascript"),
                        QStringLiteral("application/x-shellscript"),
                        QStringLiteral("application/x-yaml"),
                        QStringLiteral("application/toml"),
                        QStringLiteral("application/sql"),
                        QStringLiteral("application/x-desktop"),
                        QStringLiteral("application/x-subrip")});
}
}

QString ViewerResolver::viewerFor(const QMimeType &mime)
{
    if (!mime.isValid() || mime.inherits(QStringLiteral("inode/directory"))) {
        return QStringLiteral("Info");
    }

    static const QList<QByteArray> imageTypes = QImageReader::supportedMimeTypes();
    for (const QByteArray &type : imageTypes) {
        if (mime.inherits(QString::fromLatin1(type))) {
            return QStringLiteral("Image");
        }
    }

    if (mime.inherits(QStringLiteral("application/pdf"))) {
        return QStringLiteral("Pdf");
    }
    // Before archives: OOXML and OpenDocument files are zip containers.
    if (isOffice(mime)) {
        return QStringLiteral("Office");
    }
    if (mime.name().startsWith(QLatin1String("video/")) || mime.name().startsWith(QLatin1String("audio/"))
        || mime.inherits(QStringLiteral("application/ogg"))) {
        return QStringLiteral("Media");
    }
    if (isFont(mime)) {
        return QStringLiteral("Font");
    }
    // Before text: OBJ, glTF and ASCII STL are text formats.
    if (isModel(mime)) {
        return QStringLiteral("Model");
    }
    // Before archives: comic books are zip, rar, 7z or tar files.
    if (isComic(mime)) {
        return QStringLiteral("Comic");
    }
    if (isArchive(mime)) {
        return QStringLiteral("Archive");
    }
    if (mime.inherits(QStringLiteral("text/markdown"))) {
        return QStringLiteral("Markdown");
    }
    if (isText(mime)) {
        return QStringLiteral("Text");
    }
    return QStringLiteral("Info");
}

QString ViewerResolver::viewerForFile(const QFileInfo &file, const QMimeType &mime)
{
    const QString viewer = viewerFor(mime);
    // isFile() also keeps FIFOs and devices out: reading those could block forever.
    if (viewer != QLatin1String("Info") || !file.isFile() || file.size() == 0) {
        return viewer;
    }

    QFile handle(file.absoluteFilePath());
    if (handle.open(QIODevice::ReadOnly) && looksLikeText(handle.read(4096))) {
        return QStringLiteral("Text");
    }
    return viewer;
}

bool ViewerResolver::looksLikeText(const QByteArray &data)
{
    if (data.isEmpty() || data.contains('\0')) {
        return false;
    }
    QStringDecoder decoder(QStringDecoder::Utf8);
    // The sample may end in the middle of a multi-byte character; only judge complete input.
    // decode() is lazy: the bytes are only decoded (and errors found) when converted to a QString.
    const QString decoded = decoder.decode(data);
    Q_UNUSED(decoded)
    return !decoder.hasError();
}
