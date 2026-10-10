// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include "kpeek-version.h"
#include "pdfdocument.h"
#include "peekcontroller.h"
#include "workerpool.h"

#include <KAboutData>
#include <KCrash>
#include <KDBusService>
#include <KLocalizedQmlContext>
#include <KLocalizedString>

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>

#include <cstdlib>

namespace
{
QList<QUrl> urlsFromArguments(QCommandLineParser &parser, const QStringList &arguments, const QString &workingDirectory)
{
    parser.parse(arguments);
    QList<QUrl> urls;
    const QStringList positional = parser.positionalArguments();
    for (const QString &argument : positional) {
        urls.append(QUrl::fromUserInput(argument, workingDirectory, QUrl::AssumeLocalFile));
    }
    return urls;
}
}

int main(int argc, char *argv[])
{
    // QApplication rather than QGuiApplication so Kirigami picks up the desktop style and platform theme.
    QApplication app(argc, argv);
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

    QQmlApplicationEngine engine;
    KLocalization::setupLocalizedContext(&engine);
    engine.addImageProvider(QLatin1String(PdfPageProvider::Id), new PdfPageProvider);
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
    if (!workerPool()->waitForDone(2000)) {
        std::quick_exit(exitCode);
    }
    return exitCode;
}
