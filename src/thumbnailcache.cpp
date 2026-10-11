// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include "thumbnailcache.h"
#include "systeminfo.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QtEndian>

namespace
{
/// Value of the tEXt chunk @p key in the PNG at @p path. Reads only the chunks before the pixel data:
/// QImageReader can't be used (it truncates keys at ':', as in "Thumb::MTime") and decoding the whole
/// thumbnail just for this would cost tens of milliseconds on every file switch.
QByteArray pngText(const QString &path, const QByteArray &key)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.read(8) != QByteArray("\x89PNG\r\n\x1a\n", 8)) {
        return {};
    }
    while (!file.atEnd()) {
        const QByteArray header = file.read(8);
        if (header.size() != 8) {
            return {};
        }
        const quint32 length = qFromBigEndian<quint32>(header.constData());
        const QByteArray type = header.mid(4);
        if (type == "IDAT" || type == "IEND" || length > 1024 * 1024) {
            return {}; // text chunks come first in thumbnails; stop at the pixel data
        }
        const QByteArray data = file.read(length);
        file.skip(4); // CRC
        if (type == "tEXt" && data.startsWith(QByteArray(key + '\0'))) {
            return data.mid(key.size() + 1);
        }
    }
    return {};
}

QString defaultCacheRoot()
{
    // Inside a Flatpak, XDG_CACHE_HOME points into the sandbox; the shared cache is the host's.
    if (isFlatpak()) {
        const QString hostCache = qEnvironmentVariable("HOST_XDG_CACHE_HOME");
        return (hostCache.isEmpty() ? QDir::homePath() + QStringLiteral("/.cache") : hostCache) + QStringLiteral("/thumbnails");
    }
    return QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QStringLiteral("/thumbnails");
}
}

QString ThumbnailCache::fileNameFor(const QString &path)
{
    QString absolute = QFileInfo(path).absoluteFilePath();
    if (absolute.startsWith(QLatin1String("/run/host/")) && isFlatpak()) {
        absolute.remove(0, 9); // Dolphin knows the host's /usr/… and /etc/… by their own names (see main.cpp)
    }
    const QByteArray uri = QUrl::fromLocalFile(absolute).toEncoded();
    return QString::fromLatin1(QCryptographicHash::hash(uri, QCryptographicHash::Md5).toHex()) + QStringLiteral(".png");
}

QUrl ThumbnailCache::find(const QString &path, const QString &cacheRoot)
{
    const QFileInfo file(path);
    if (!file.isFile()) {
        return {};
    }
    const QString root = cacheRoot.isEmpty() ? defaultCacheRoot() : cacheRoot;
    const QString name = fileNameFor(path);
    const QByteArray mtime = QByteArray::number(file.lastModified().toSecsSinceEpoch());

    for (const char *size : {"xx-large", "x-large", "large", "normal"}) {
        const QString candidate = root + QLatin1Char('/') + QLatin1String(size) + QLatin1Char('/') + name;
        if (!QFileInfo::exists(candidate)) {
            continue;
        }
        // The spec stores the source's mtime in the PNG; a mismatch means the file changed since.
        if (pngText(candidate, "Thumb::MTime") == mtime) {
            return QUrl::fromLocalFile(candidate);
        }
    }
    return {};
}
