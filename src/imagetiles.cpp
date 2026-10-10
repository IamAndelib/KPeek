// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include "imagetiles.h"
#include "workerpool.h"

#include <QImageReader>
#include <QQuickTextureFactory>
#include <QRunnable>
#include <QTransform>
#include <QUrl>

#include <atomic>

namespace
{
constexpr qint64 MaxUnclippedPixels = 64'000'000;

QImage oriented(QImage image, QImageIOHandler::Transformations transformation)
{
    Qt::Orientations flips;
    if (transformation & QImageIOHandler::TransformationMirror) {
        flips |= Qt::Horizontal;
    }
    if (transformation & QImageIOHandler::TransformationFlip) {
        flips |= Qt::Vertical;
    }
    if (flips) {
        image = std::move(image).flipped(flips);
    }
    if (transformation & QImageIOHandler::TransformationRotate90) {
        image = image.transformed(QTransform().rotate(90));
    }
    return image;
}

class TileResponse : public QQuickImageResponse, public QRunnable
{
public:
    TileResponse(const QString &path, const QSize &scaledSize, const QRect &clip)
        : m_path(path)
        , m_scaledSize(scaledSize)
        , m_clip(clip)
    {
        setAutoDelete(false);
    }

    void run() override
    {
        if (!m_cancelled) {
            m_image = ImageTiles::decode(m_path, m_scaledSize, m_clip);
        }
        Q_EMIT finished();
    }

    void cancel() override
    {
        m_cancelled = true; // the user zoomed or panned on before this tile was needed
    }

    QQuickTextureFactory *textureFactory() const override
    {
        return QQuickTextureFactory::textureFactoryForImage(m_image);
    }

private:
    QString m_path;
    QSize m_scaledSize;
    QRect m_clip;
    QImage m_image;
    std::atomic_bool m_cancelled = false;
};
}

QRect ImageTiles::mapToFile(const QRect &displayed, const QSize &displayedSize, QImageIOHandler::Transformations transformation)
{
    QRect rect = displayed;
    QSize size = displayedSize;
    // Undo the rotation: a 90° clockwise turn maps (x, y, w, h) in W×H to (H - y - h, x, h, w) in H×W.
    if (transformation & QImageIOHandler::TransformationRotate90) {
        size.transpose();
        rect = QRect(rect.y(), size.height() - rect.x() - rect.width(), rect.height(), rect.width());
    }
    // Mirroring and flipping are their own inverses.
    if (transformation & QImageIOHandler::TransformationMirror) {
        rect.moveLeft(size.width() - rect.x() - rect.width());
    }
    if (transformation & QImageIOHandler::TransformationFlip) {
        rect.moveTop(size.height() - rect.y() - rect.height());
    }
    return rect;
}

QImage ImageTiles::decode(const QString &path, const QSize &scaledSize, const QRect &clip)
{
    QImageReader reader(path);
    reader.setAutoTransform(false); // we orient the small tile ourselves
    const QImageIOHandler::Transformations transformation = reader.transformation();
    QSize fileScaledSize = scaledSize;
    if (transformation & QImageIOHandler::TransformationRotate90) {
        fileScaledSize.transpose();
    }
    reader.setScaledSize(fileScaledSize);
    reader.setScaledClipRect(mapToFile(clip, scaledSize, transformation) & QRect(QPoint(), fileScaledSize));
    return oriented(reader.read(), transformation);
}

QString ImageTiles::source(const QString &path, qint64 mtime, const QSize &scaledSize, const QRect &clip)
{
    // The mtime makes the URL change when the file does, so Qt's image cache never serves a stale tile.
    return QStringLiteral("image://%1/%2x%3/%4,%5,%6,%7/%8/%9")
        .arg(QLatin1String(ProviderId))
        .arg(scaledSize.width())
        .arg(scaledSize.height())
        .arg(clip.x())
        .arg(clip.y())
        .arg(clip.width())
        .arg(clip.height())
        .arg(mtime)
        .arg(QString::fromLatin1(QUrl::toPercentEncoding(path)));
}

bool ImageTiles::cheapToTile(const QString &path)
{
    QImageReader reader(path);
    const QSize size = reader.size();
    return reader.supportsOption(QImageIOHandler::ScaledClipRect) || qint64(size.width()) * size.height() <= MaxUnclippedPixels;
}

QQuickImageResponse *TileProvider::requestImageResponse(const QString &id, const QSize &)
{
    // id is "<W>x<H>/<x>,<y>,<w>,<h>/<mtime>/<percent-encoded path>"; the path may arrive already decoded.
    const QStringList size = id.section(QLatin1Char('/'), 0, 0).split(QLatin1Char('x'));
    const QStringList clip = id.section(QLatin1Char('/'), 1, 1).split(QLatin1Char(','));
    const QString path = QUrl::fromPercentEncoding(id.section(QLatin1Char('/'), 3, -1).toUtf8());
    auto *response = new TileResponse(path,
                                      QSize(size.value(0).toInt(), size.value(1).toInt()),
                                      QRect(clip.value(0).toInt(), clip.value(1).toInt(), clip.value(2).toInt(), clip.value(3).toInt()));
    workerPool()->start(response);
    return response;
}
