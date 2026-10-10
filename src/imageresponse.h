// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#pragma once

#include <QImage>
#include <QQuickImageResponse>
#include <QQuickTextureFactory>
#include <QRunnable>

#include <atomic>
#include <functional>

/**
 * An image made on the worker pool for an async image provider (PDF pages, comic pages).
 * The render function gets the cancel flag, set when the page is scrolled past or re-requested
 * at another zoom level, so it can give up early.
 */
class ImageResponse : public QQuickImageResponse, public QRunnable
{
public:
    using Render = std::function<QImage(const std::atomic_bool &cancelled)>;

    explicit ImageResponse(Render render)
        : m_render(std::move(render))
    {
        setAutoDelete(false); // Qt deletes the response once it has handled finished()
    }

    void run() override
    {
        if (!m_cancelled) {
            m_image = m_render(m_cancelled);
        }
        Q_EMIT finished();
    }

    void cancel() override
    {
        m_cancelled = true;
    }

    QQuickTextureFactory *textureFactory() const override
    {
        return QQuickTextureFactory::textureFactoryForImage(m_image);
    }

    // Without an error, Qt would show a page that failed to render as a loaded, blank one.
    QString errorString() const override
    {
        return m_image.isNull() && !m_cancelled ? QStringLiteral("The page could not be rendered") : QString();
    }

private:
    Render m_render;
    QImage m_image;
    std::atomic_bool m_cancelled = false;
};
