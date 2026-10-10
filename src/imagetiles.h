// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#pragma once

#include <QImage>
#include <QImageIOHandler>
#include <QQuickAsyncImageProvider>
#include <QRect>

/**
 * Decodes just the visible part of an image at the current zoom ("detail tile").
 *
 * The image viewer shows one screen-sized decode of the whole image and, when zoomed in
 * past what that resolves, one tile for the visible area on top. Memory stays at about two
 * screens' worth of pixels however large the image or the zoom.
 *
 * Source: "image://kpeektile/<W>x<H>/<x>,<y>,<w>,<h>/<mtime>/<percent-encoded path>", where W×H is
 * the size the whole image is shown at and x,y,w,h the visible part, both in displayed (EXIF-rotated)
 * device pixels.
 */
namespace ImageTiles
{
inline constexpr auto ProviderId = "kpeektile";

/// Maps @p displayed, a rect in the displayed (oriented) image of size @p displayedSize, to the same
/// pixels in the stored file's orientation. Qt applies Mirror/Flip first, then a 90° clockwise rotation.
QRect mapToFile(const QRect &displayed, const QSize &displayedSize, QImageIOHandler::Transformations transformation);

/// Decodes the @p clip part of the image at @p path as if it were shown at @p scaledSize (both displayed coordinates).
QImage decode(const QString &path, const QSize &scaledSize, const QRect &clip);

/// Image source URL for the @p clip part of @p path shown at @p scaledSize (see the format above).
QString source(const QString &path, qint64 mtime, const QSize &scaledSize, const QRect &clip);

/// Whether tiles can be decoded cheaply. Formats without native clipping decode the whole image for
/// every tile, which is fine for normal photos but not for huge ones.
bool cheapToTile(const QString &path);
}

class TileProvider : public QQuickAsyncImageProvider
{
public:
    QQuickImageResponse *requestImageResponse(const QString &id, const QSize &requestedSize) override;
};
