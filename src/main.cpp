// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include "comicbook.h"
#include "filenavigator.h"
#include "forwarder.h"
#include "kpeek-version.h"
#include "pdfdocument.h"
#include "peekcontroller.h"
#include "systeminfo.h"
#include "workerpool.h"

#include <KAboutData>
#include <KCrash>
#include <KDBusService>
#include <KLocalizedQmlContext>
#include <KLocalizedString>

#include <QApplication>
#include <QCommandLineParser>
#include <QDeadlineTimer>
#include <QDir>
#include <QIcon>
#include <QImageReader>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QQmlApplicationEngine>
#include <QQmlNetworkAccessManagerFactory>
#include <QQuickStyle>
#include <QQuickWindow>

#include <cstdlib>

#ifdef __GLIBC__
#include <malloc.h>
#endif

namespace
{
/// KPeek never needs the network. Markdown images and rich text could otherwise fetch remote URLs
/// (tracking pixels) just because a file was selected: everything but local files is refused.
class LocalOnlyNetworkAccessManager : public QNetworkAccessManager
{
protected:
    QNetworkReply *createRequest(Operation op, const QNetworkRequest &request, QIODevice *data) override
    {
        const QString scheme = request.url().scheme();
        if (scheme == QLatin1String("file") || scheme == QLatin1String("qrc") || scheme == QLatin1String("data")) {
            return QNetworkAccessManager::createRequest(op, request, data);
        }
        return QNetworkAccessManager::createRequest(op, QNetworkRequest(QUrl(QStringLiteral("blocked:"))), data);
    }
};

class LocalOnlyNetworkAccessManagerFactory : public QQmlNetworkAccessManagerFactory
{
public:
    QNetworkAccessManager *create(QObject *parent) override
    {
        auto *manager = new LocalOnlyNetworkAccessManager;
        manager->setParent(parent);
        return manager;
    }
};

QList<QUrl> urlsFromArguments(QCommandLineParser &parser, const QStringList &arguments, const QString &workingDirectory)
{
    parser.parse(arguments);
    QList<QUrl> urls;
    const QStringList positional = parser.positionalArguments();
    for (const QString &argument : positional) {
        if (argument.isEmpty()) {
            continue; // Dolphin passes an empty URL when nothing is selected
        }
        QUrl url = trashedFileUrl(QUrl::fromUserInput(argument, workingDirectory, QUrl::AssumeLocalFile), homeTrash());
        // The Flatpak runtime has its own /usr and /etc; the host's are under /run/host.
        if (isFlatpak() && url.isLocalFile()) {
            const QString path = QDir::cleanPath(url.toLocalFile());
            if (path.startsWith(QLatin1String("/usr/")) || path.startsWith(QLatin1String("/etc/"))) {
                url = QUrl::fromLocalFile(QStringLiteral("/run/host") + path);
            }
        }
        urls.append(url);
    }
    return urls;
}
}

int main(int argc, char *argv[])
{
#ifdef __GLIBC__
    // glibc gives threads their own heaps ("arenas"), up to 8 per core. Each video preview's decoder
    // threads left frame-sized free memory in fresh arenas that malloc_trim() can't return: 270 MB
    // stayed resident after three rounds of videos. Two arenas: 78 MB, at no measurable CPU cost.
    mallopt(M_ARENA_MAX, 2);
#endif
    if (const auto status = forwardToRunningInstance(argc, argv, KPEEK_APP_ID)) {
        return *status;
    }
    // QApplication rather than QGuiApplication so Kirigami picks up the desktop style and platform theme.
    QApplication app(argc, argv); // sets the C locale from the environment
    ensureUtf8Locale();
    if (isLowMemorySystem()) {
        // An image over this fails cleanly (to the thumbnail) instead of pushing a 4 GB machine into swap.
        QImageReader::setAllocationLimit(128);
    }
    app.setQuitOnLastWindowClosed(false);

    KLocalizedString::setApplicationDomain("kpeek");

    KAboutData about(QStringLiteral("kpeek"),
                     i18nc("@title", "KPeek"),
                     QStringLiteral(KPEEK_VERSION_STRING),
                     i18nc("@info", "Quick file previews for Dolphin"),
                     KAboutLicense::GPL_V2,
                     i18nc("@info:credit", "© 2026 Andelib Taref Srizon"));
    about.addAuthor(i18nc("@info:credit", "Andelib Taref Srizon"), QString(), QStringLiteral("andelibtarefsrizon@gmail.com"));
    about.setOrganizationDomain("iamandelib.github.io");
    about.setDesktopFileName(QStringLiteral(KPEEK_APP_ID));
    KAboutData::setApplicationData(about);
    KCrash::initialize();
    // An installed theme icon wins; otherwise the copy built into the binary, so even an uninstalled
    // build (or a desktop without our .desktop file) shows the right icon in the taskbar.
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral(KPEEK_APP_ID), QIcon(QStringLiteral(":/icon.svg"))));

    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    }

    QCommandLineParser parser;
    about.setupCommandLine(&parser);
    parser.addPositionalArgument(QStringLiteral("files"), i18nc("@info:shell", "Files or folders to preview."), QStringLiteral("[files...]"));
    parser.process(app);
    about.processCommandLine(&parser);

    const QList<QUrl> urls = urlsFromArguments(parser, app.arguments(), QDir::currentPath());
    if (urls.isEmpty()) {
        parser.showHelp(1);
    }

    // Registers io.github.iamandelib.kpeek; a second invocation forwards its arguments and exits here.
    KDBusService service(KDBusService::Unique);

    PeekController controller(nullptr);

    LocalOnlyNetworkAccessManagerFactory networkFactory; // must outlive the engine
    QQmlApplicationEngine engine;
    KLocalization::setupLocalizedContext(&engine);
    engine.setNetworkAccessManagerFactory(&networkFactory);
    engine.addImageProvider(QLatin1String(PdfPageProvider::Id), new PdfPageProvider);
    engine.addImageProvider(QLatin1String(ArchiveImageProvider::Id), new ArchiveImageProvider);
    engine.loadFromModule("io.github.iamandelib.kpeek", "Main");
    auto *window = engine.rootObjects().isEmpty() ? nullptr : qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
    if (!window) {
        return 1;
    }
    controller.setWindow(window);

    QObject::connect(&service, &KDBusService::activateRequested, &controller, [&parser, &controller](const QStringList &arguments, const QString &cwd) {
        controller.showFiles(urlsFromArguments(parser, arguments, cwd));
    });

    controller.showFiles(urls, false);
    const int exitCode = app.exec();

    // Don't let quitting hang on background work that can't be interrupted (e.g. decompressing
    // a huge tarball): that would keep the D-Bus name, so new previews would go to a dying process.
    workerPool()->clear();
    archivePool()->clear();
    QDeadlineTimer deadline(2000);
    if (!workerPool()->waitForDone(deadline) || !archivePool()->waitForDone(deadline)) {
        std::quick_exit(exitCode);
    }
    return exitCode;
}
