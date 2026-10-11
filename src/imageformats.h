// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#pragma once

#include <QCoreApplication>
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QPluginLoader>
#include <QSet>

struct ImageFormats {
    QSet<QByteArray> keys; ///< file suffixes, as QImageReader::supportedImageFormats()
    QSet<QByteArray> mimeTypes; ///< as QImageReader::supportedMimeTypes()
};

/**
 * The image formats QImageReader can read, minus EPS. QImageReader's own lists load every image plugin
 * and its codecs (FFmpeg, AV1, OpenEXR, LibRaw…): 27 ms of a 275 ms start. The plugins' metadata
 * holds the same lists and is read without loading them (2 ms).
 */
inline const ImageFormats &imageFormats()
{
    static const ImageFormats formats = [] {
        ImageFormats f;
        // Built into QtGui rather than plugins.
        f.keys = {"bmp", "pbm", "pgm", "png", "ppm", "xbm", "xpm"};
        f.mimeTypes =
            {"image/bmp", "image/x-portable-bitmap", "image/x-portable-graymap", "image/png", "image/x-portable-pixmap", "image/x-xbitmap", "image/x-xpixmap"};
        const QStringList paths = QCoreApplication::libraryPaths();
        for (const QString &path : paths) {
            const QDir dir(path + QStringLiteral("/imageformats"));
            const QStringList files = dir.entryList(QDir::Files);
            for (const QString &file : files) {
                const QJsonObject metaData = QPluginLoader(dir.filePath(file)).metaData().value(QLatin1String("MetaData")).toObject();
                const QJsonArray keys = metaData.value(QLatin1String("Keys")).toArray();
                const QJsonArray mimeTypes = metaData.value(QLatin1String("MimeTypes")).toArray();
                // Parallel lists, read pairwise like QImageReader does: Qt's JPEG 2000 plugin lists more MIME types
                // than keys, and the extra video/mj2 is a video no image reader opens.
                for (qsizetype i = 0; i < keys.size(); ++i) {
                    f.keys.insert(keys.at(i).toString().toLatin1());
                    if (i < mimeTypes.size()) {
                        f.mimeTypes.insert(mimeTypes.at(i).toString().toLatin1());
                    }
                }
            }
        }
        // kimageformats reads EPS by running Ghostscript: never on untrusted files, never on the host.
        for (const char *key : {"eps", "epsf", "epsi"}) {
            f.keys.remove(key);
        }
        f.mimeTypes.remove("image/x-eps");
        f.mimeTypes.remove("application/postscript");
        return f;
    }();
    return formats;
}
