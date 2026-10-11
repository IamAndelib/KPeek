// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include <QBuffer>
#include <QFile>
#include <QImageReader>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

class ExecGuardTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testGhostscriptNeverStarts();
    void testDisguisedPostScript();
};

// The EPS image plugin starts Ghostscript through QProcess; src/execguard.cpp must stop it. A program named gs
// (here a link to true) must not start, while the same program under another name still does.
void ExecGuardTest::testGhostscriptNeverStarts()
{
    const QString truePath = QStandardPaths::findExecutable(QStringLiteral("true"));
    QVERIFY(!truePath.isEmpty());
    QTemporaryDir dir;
    QVERIFY(QFile::link(truePath, dir.filePath(QStringLiteral("gs"))));
    QVERIFY(QFile::link(truePath, dir.filePath(QStringLiteral("other"))));

    QCOMPARE(QProcess::execute(dir.filePath(QStringLiteral("other")), {}), 0);
    QCOMPARE(QProcess::execute(dir.filePath(QStringLiteral("gs")), {}), -2); // -2: could not be started

    QProcess process;
    process.start(dir.filePath(QStringLiteral("gs")), {});
    QVERIFY(!process.waitForStarted());
    QCOMPARE(process.error(), QProcess::FailedToStart);
}

// The real attack: PostScript in a file that claims to be something else reaches the EPS plugin through Qt's
// content sniffing, which then starts gs from PATH. Here gs is a script that leaves a marker.
void ExecGuardTest::testDisguisedPostScript()
{
    if (!QImageReader::supportedImageFormats().contains("eps")) {
        QSKIP("No EPS image plugin installed");
    }
    QTemporaryDir dir;
    const QString marker = dir.filePath(QStringLiteral("ran"));
    QFile script(dir.filePath(QStringLiteral("gs")));
    QVERIFY(script.open(QIODevice::WriteOnly));
    script.write("#!/bin/sh\ntouch '" + QFile::encodeName(marker) + "'\n");
    script.close();
    QVERIFY(script.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
    const QByteArray path = qgetenv("PATH");
    qputenv("PATH", QByteArray(QFile::encodeName(dir.path()) + ':' + path));

    QByteArray postScript("%!PS-Adobe-3.0 EPSF-3.0\n%%BoundingBox: 0 0 10 10\nshowpage\n");
    QBuffer buffer(&postScript);
    QImageReader reader(&buffer); // no format: detected from the content, as for comic pages
    reader.read();
    qputenv("PATH", path);
    QVERIFY(!QFile::exists(marker));
}

QTEST_GUILESS_MAIN(ExecGuardTest)

#include "execguardtest.moc"
