// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include "viewerresolver.h"

#include <QMimeDatabase>
#include <QTest>

class ViewerResolverTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testViewer_data();
    void testViewer();
    void testLooksLikeText();
};

void ViewerResolverTest::testViewer_data()
{
    QTest::addColumn<QString>("mimeType");
    QTest::addColumn<QString>("viewer");

    QTest::newRow("png") << QStringLiteral("image/png") << QStringLiteral("Image");
    QTest::newRow("jpeg") << QStringLiteral("image/jpeg") << QStringLiteral("Image");
    QTest::newRow("folder") << QStringLiteral("inode/directory") << QStringLiteral("Info");
    QTest::newRow("unknown") << QStringLiteral("application/octet-stream") << QStringLiteral("Info");
    QTest::newRow("pdf") << QStringLiteral("application/pdf") << QStringLiteral("Pdf");
    QTest::newRow("docx is office, not a zip archive")
        << QStringLiteral("application/vnd.openxmlformats-officedocument.wordprocessingml.document") << QStringLiteral("Office");
    QTest::newRow("odt") << QStringLiteral("application/vnd.oasis.opendocument.text") << QStringLiteral("Office");
    QTest::newRow("doc") << QStringLiteral("application/msword") << QStringLiteral("Office");
    QTest::newRow("mp4") << QStringLiteral("video/mp4") << QStringLiteral("Media");
    QTest::newRow("mp3") << QStringLiteral("audio/mpeg") << QStringLiteral("Media");
    QTest::newRow("zip") << QStringLiteral("application/zip") << QStringLiteral("Archive");
    QTest::newRow("tar.gz") << QStringLiteral("application/x-compressed-tar") << QStringLiteral("Archive");
    QTest::newRow("ttf") << QStringLiteral("font/ttf") << QStringLiteral("Font");
    QTest::newRow("markdown") << QStringLiteral("text/markdown") << QStringLiteral("Markdown");
    QTest::newRow("c++") << QStringLiteral("text/x-c++src") << QStringLiteral("Text");
    QTest::newRow("json") << QStringLiteral("application/json") << QStringLiteral("Text");
    QTest::newRow("shell script") << QStringLiteral("application/x-shellscript") << QStringLiteral("Text");
}

void ViewerResolverTest::testViewer()
{
    QFETCH(QString, mimeType);
    QFETCH(QString, viewer);
    QCOMPARE(ViewerResolver::viewerFor(QMimeDatabase().mimeTypeForName(mimeType)), viewer);
}

void ViewerResolverTest::testLooksLikeText()
{
    QVERIFY(ViewerResolver::looksLikeText("hello\nworld\n"));
    QVERIFY(ViewerResolver::looksLikeText("caf\xc3\xa9")); // UTF-8 é
    QVERIFY(ViewerResolver::looksLikeText("caf\xc3")); // sample cut inside a character
    QVERIFY(!ViewerResolver::looksLikeText(QByteArray("bin\0ary", 7)));
    QVERIFY(!ViewerResolver::looksLikeText("\xff\xfe\xfa"));
    QVERIFY(!ViewerResolver::looksLikeText(""));
}

QTEST_GUILESS_MAIN(ViewerResolverTest)

#include "viewerresolvertest.moc"
