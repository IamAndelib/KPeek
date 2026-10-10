// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include "peekcontroller.h"
#include "imagetiles.h"
#include "thumbnailcache.h"
#include "viewerresolver.h"

#include <KFormat>
#include <KLocalizedString>
#include <KWindowSystem>

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QImageReader>
#include <QMimeDatabase>
#include <QQuickWindow>

using namespace std::chrono_literals;

namespace
{
PeekController *s_instance = nullptr;

// How long the hidden window stays resident so the next preview opens instantly.
constexpr auto IdleQuitTimeout = 10min;
}

PeekController::PeekController(QObject *parent)
    : QObject(parent)
{
    Q_ASSERT(!s_instance);
    s_instance = this;

    m_idleQuitTimer.setSingleShot(true);
    m_idleQuitTimer.setInterval(IdleQuitTimeout);
    connect(&m_idleQuitTimer, &QTimer::timeout, qApp, &QCoreApplication::quit);
}

PeekController *PeekController::instance()
{
    return s_instance;
}

PeekController *PeekController::create(QQmlEngine *, QJSEngine *)
{
    QQmlEngine::setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
    return s_instance;
}

void PeekController::setWindow(QQuickWindow *window)
{
    m_window = window;
}

void PeekController::showFiles(const QList<QUrl> &urls, bool toggle)
{
    if (urls.isEmpty()) {
        return;
    }

    if (toggle && m_window && m_window->isVisible() && m_navigator.matchesSelection(urls)) {
        close();
        return;
    }

    m_navigator.setSelection(urls);
    updateCurrent();
    raiseWindow();
}

void PeekController::next()
{
    m_navigator.step(1);
    updateCurrent();
}

void PeekController::previous()
{
    m_navigator.step(-1);
    updateCurrent();
}

void PeekController::close()
{
    if (m_window) {
        m_window->hide();
    }
    m_idleQuitTimer.start();
}

void PeekController::openExternally()
{
    if (!url().isEmpty()) {
        QDesktopServices::openUrl(url());
        close();
    }
}

void PeekController::raiseWindow()
{
    m_idleQuitTimer.stop();
    if (!m_window) {
        return;
    }
    // KDBusService puts the invoking launcher's activation token in the environment,
    // which lets Wayland compositors give the already-running window focus.
    KWindowSystem::updateStartupId(m_window);
    m_window->show();
    m_window->raise();
    KWindowSystem::activateWindow(m_window);
}

void PeekController::updateCurrent()
{
    const QUrl current = m_navigator.current();
    const QFileInfo info(current.toLocalFile());
    const QMimeType mime = QMimeDatabase().mimeTypeForUrl(current);

    // Remote and virtual URLs (smb:, trash:, http:) only get the info card: viewers read local files.
    m_viewer = current.isLocalFile() ? ViewerResolver::viewerForFile(info, mime) : QStringLiteral("Info");
    m_mimeType = mime.name();
    m_mimeComment = mime.comment();
    m_iconName = mime.iconName();
    m_isFolder = info.isDir();
    m_modified = info.lastModified();

    m_thumbnail = m_viewer == QLatin1String("Image") || m_viewer == QLatin1String("Media") ? ThumbnailCache::find(info.absoluteFilePath()) : QUrl();
    m_imageSize = {};
    m_animated = false;
    m_tileable = false;
    if (m_viewer == QLatin1String("Image")) {
        // Reads only the header: cheap even for huge photos.
        QImageReader reader(info.absoluteFilePath());
        m_imageSize = reader.size();
        if (reader.transformation() & QImageIOHandler::TransformationRotate90) {
            m_imageSize.transpose();
        }
        m_animated = reader.supportsAnimation() && reader.imageCount() != 1; // 0 means "unknown"
        m_tileable = !m_animated && ImageTiles::cheapToTile(info.absoluteFilePath());
    }

    if (m_isFolder) {
        int entries = 0;
        for (QDirIterator it(info.absoluteFilePath(), QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System); it.hasNext(); it.next()) {
            ++entries;
        }
        m_sizeText = i18ncp("@info folder contents", "%1 item", "%1 items", entries);
    } else {
        m_sizeText = KFormat().formatByteSize(info.size());
    }

    Q_EMIT currentChanged();
}

QUrl PeekController::url() const
{
    return m_navigator.current();
}

QString PeekController::fileName() const
{
    return url().fileName();
}

QString PeekController::viewer() const
{
    return m_viewer;
}

QString PeekController::mimeType() const
{
    return m_mimeType;
}

QString PeekController::mimeComment() const
{
    return m_mimeComment;
}

QString PeekController::iconName() const
{
    return m_iconName;
}

bool PeekController::isFolder() const
{
    return m_isFolder;
}

QString PeekController::sizeText() const
{
    return m_sizeText;
}

QDateTime PeekController::modified() const
{
    return m_modified;
}

int PeekController::index() const
{
    return m_navigator.currentIndex();
}

int PeekController::count() const
{
    return m_navigator.count();
}

QSize PeekController::imageSize() const
{
    return m_imageSize;
}

bool PeekController::animated() const
{
    return m_animated;
}

QUrl PeekController::thumbnail() const
{
    return m_thumbnail;
}

QString PeekController::tileSource(const QSize &scaledSize, const QRect &clip) const
{
    return ImageTiles::source(url().toLocalFile(), m_modified.toMSecsSinceEpoch(), scaledSize, clip);
}

bool PeekController::tileable() const
{
    return m_tileable;
}

#include "moc_peekcontroller.cpp"
