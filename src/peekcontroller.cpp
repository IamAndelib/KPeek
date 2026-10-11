// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include "peekcontroller.h"
#include "systeminfo.h"
#include "thumbnailcache.h"
#include "viewerresolver.h"
#include "workerpool.h"

#include <KFormat>
#include <KLocalizedString>
#include <KWindowShadow>
#include <KWindowSystem>

#include <QCoreApplication>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QImageReader>
#include <QMimeDatabase>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QQuickWindow>
#include <QtConcurrentRun>

#include <cmath>

namespace
{
PeekController *s_instance = nullptr;

/// A soft drop shadow for the frameless window, drawn by the compositor (Plasma; elsewhere it's ignored).
/// One image whose centre pixel stands for the window, sliced into the eight tiles around it.
void addShadow(KWindowShadow *shadow)
{
    constexpr int radius = 22;
    constexpr int size = radius * 2 + 1;
    QImage image(size, size, QImage::Format_ARGB32_Premultiplied);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const double distance = std::hypot(x - radius, y - radius) / radius;
            const double strength = distance >= 1 ? 0 : std::pow(1 - distance, 2) * 0.32;
            image.setPixelColor(x, y, QColor(0, 0, 0, int(strength * 255)));
        }
    }
    const auto tile = [&image](int x, int y, int w, int h) {
        auto t = KWindowShadowTile::Ptr::create();
        t->setImage(image.copy(x, y, w, h));
        return t;
    };
    shadow->setTopLeftTile(tile(0, 0, radius, radius));
    shadow->setTopTile(tile(radius, 0, 1, radius));
    shadow->setTopRightTile(tile(radius + 1, 0, radius, radius));
    shadow->setLeftTile(tile(0, radius, radius, 1));
    shadow->setRightTile(tile(radius + 1, radius, radius, 1));
    shadow->setBottomLeftTile(tile(0, radius + 1, radius, radius));
    shadow->setBottomTile(tile(radius, radius + 1, 1, radius));
    shadow->setBottomRightTile(tile(radius + 1, radius + 1, radius, radius));
    shadow->setPadding(QMargins(radius, radius, radius, radius));
}
}

PeekController::PeekController(QObject *parent)
    : QObject(parent)
{
    Q_ASSERT(!s_instance);
    s_instance = this;
    addShadow(&m_shadow);

    // setFuture() drops the results of earlier, abandoned requests; takeResult() moves a 100k-entry
    // listing out of the future instead of copying it.
    connect(&m_siblingsWatcher, &QFutureWatcher<FileNavigator::Siblings>::finished, this, [this] {
        m_navigator.setSiblings(m_siblingsWatcher.future().takeResult());
        Q_EMIT positionChanged();
    });
    connect(&m_folderSizeWatcher, &QFutureWatcher<FolderSize>::finished, this, [this] {
        const FolderSize size = m_folderSizeWatcher.future().takeResult();
        if (m_isFolder && size.path == QFileInfo(url().toLocalFile()).absoluteFilePath()) { // not a stale count
            m_sizeText = size.text;
            Q_EMIT sizeTextChanged();
        }
    });
}

PeekController *PeekController::create(QQmlEngine *, QJSEngine *)
{
    QQmlEngine::setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
    return s_instance;
}

void PeekController::setWindow(QQuickWindow *window)
{
    m_window = window;
    window->installEventFilter(this);
    // Called on the render thread with the OpenGL context current (the default backend on Linux),
    // where the GPU can be asked; the property is set on this thread. Other backends keep the guess.
    connect(
        window,
        &QQuickWindow::sceneGraphInitialized,
        this,
        [this] {
            GLint limit = 0;
            if (QOpenGLContext *context = QOpenGLContext::currentContext()) {
                context->functions()->glGetIntegerv(GL_MAX_TEXTURE_SIZE, &limit);
            }
            if (limit > 0) {
                QMetaObject::invokeMethod(this, [this, limit] {
                    m_maxTextureSize = limit;
                    Q_EMIT maxTextureSizeChanged();
                });
            }
        },
        Qt::DirectConnection);
}

bool PeekController::eventFilter(QObject *watched, QEvent *event)
{
    // Also sent when the window moves to a monitor with another scale.
    if (watched == m_window && event->type() == QEvent::DevicePixelRatioChange) {
        Q_EMIT devicePixelRatioChanged();
    }
    return QObject::eventFilter(watched, event);
}

void PeekController::showFiles(const QList<QUrl> &urls, bool toggle)
{
    if (urls.isEmpty()) {
        return;
    }

    if (toggle && m_showing && m_navigator.matchesSelection(urls)) {
        close();
        return;
    }

    m_navigator.setSelection(urls);
    if (m_navigator.siblingsNeeded()) {
        // Shown alone right away; the arrows work once the folder is listed.
        m_siblingsWatcher.setFuture(QtConcurrent::run(workerPool(), &FileNavigator::listSiblings, urls.first()));
    }
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
    if (!m_showing) {
        return;
    }
    m_showing = false;
    Q_EMIT showingChanged(); // unloads the viewer, stopping playback
    // Nothing stays behind between previews: no background process, no tray entry, no memory.
    if (m_window) {
        m_window->hide();
    }
    QCoreApplication::quit();
}

void PeekController::openExternally()
{
    if (url().isEmpty()) {
        return;
    }
    // Keep the preview when nothing could open it, rather than leave the user with nothing.
    if (QDesktopServices::openUrl(url())) {
        close();
    } else {
        qWarning() << "No application could open" << url();
    }
}

void PeekController::raiseWindow()
{
    if (!m_window) {
        return;
    }
    if (!m_showing) {
        m_showing = true;
        Q_EMIT showingChanged();
    }
    // KDBusService puts the invoking launcher's activation token in the environment,
    // which lets Wayland compositors give the already-running window focus.
    KWindowSystem::updateStartupId(m_window);
    m_window->show();
    if (!m_shadow.isCreated()) {
        m_shadow.setWindow(m_window);
        m_shadow.create(); // needs the native window, which exists once shown
    }
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
    m_mimeComment = mime.comment();
    m_iconName = mime.iconName();
    m_isFolder = info.isDir();
    m_fileSize = info.isFile() ? info.size() : 0;
    m_modified = info.lastModified();

    // Shown while images and video load, and in place of the icon for files without a viewer.
    const bool wantsThumbnail = m_viewer == QLatin1String("Image") || m_viewer == QLatin1String("Media") || m_viewer == QLatin1String("Office")
        || (m_viewer == QLatin1String("Info") && info.isFile());
    m_thumbnail = wantsThumbnail ? ThumbnailCache::find(info.absoluteFilePath()) : QUrl();
    m_imageSize = {};
    m_animated = false;
    if (m_viewer == QLatin1String("Image")) {
        // Reads only the header: cheap even for huge photos.
        QImageReader reader(info.absoluteFilePath());
        m_imageSize = reader.size();
        if (reader.transformation() & QImageIOHandler::TransformationRotate90) {
            m_imageSize.transpose();
        }
        m_animated = reader.supportsAnimation() && reader.imageCount() != 1; // 0 means "unknown"
    }

    if (m_isFolder) {
        // Counting a huge folder (or one on a network share) takes a while: off the GUI thread.
        m_sizeText = QStringLiteral("…");
        m_folderSizeWatcher.setFuture(QtConcurrent::run(workerPool(), [path = info.absoluteFilePath()] {
            if (!QFileInfo(path).isReadable()) {
                return FolderSize{path, i18nc("@info folder contents", "Not readable")};
            }
            int entries = 0;
            for (QDirIterator it(path, QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System); it.hasNext(); it.next()) {
                ++entries;
            }
            return FolderSize{path, i18ncp("@info folder contents", "%1 item", "%1 items", entries)};
        }));
    } else {
        m_sizeText = current.isLocalFile() ? KFormat().formatByteSize(info.size()) : QString();
    }

    Q_EMIT currentChanged();
    Q_EMIT positionChanged();
    Q_EMIT sizeTextChanged();
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

bool PeekController::showing() const
{
    return m_showing;
}

qint64 PeekController::fileSize() const
{
    return m_fileSize;
}

QSize PeekController::imageSize() const
{
    return m_imageSize;
}

bool PeekController::animated() const
{
    return m_animated;
}

qreal PeekController::devicePixelRatio() const
{
    return m_window ? m_window->devicePixelRatio() : 1.0;
}

int PeekController::maxTextureSize() const
{
    return m_maxTextureSize;
}

bool PeekController::lowMemory() const
{
    return isLowMemorySystem();
}

QUrl PeekController::thumbnail() const
{
    return m_thumbnail;
}

#include "moc_peekcontroller.cpp"
