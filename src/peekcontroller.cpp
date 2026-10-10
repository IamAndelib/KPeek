// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include "peekcontroller.h"
#include "viewerresolver.h"

#include <KFormat>
#include <KLocalizedString>
#include <KWindowSystem>

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
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

    m_viewer = ViewerResolver::viewerForFile(info.absoluteFilePath(), mime);
    m_mimeType = mime.name();
    m_mimeComment = mime.comment();
    m_iconName = mime.iconName();
    m_isFolder = info.isDir();
    m_modified = info.lastModified();

    if (m_isFolder) {
        const int entries = QDir(info.absoluteFilePath()).count() - 2; // minus "." and ".."
        m_sizeText = i18ncp("@info folder contents", "%1 item", "%1 items", qMax(0, entries));
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

#include "moc_peekcontroller.cpp"
