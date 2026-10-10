// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include "viewerresolver.h"

#include <QMimeDatabase>
#include <QTemporaryDir>
#include <QTest>

#include <sys/stat.h>

class ViewerResolverTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testViewer_data();
    void testViewer();
    void testLooksLikeText();
    void testViewerForFile();
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
    QTest::newRow("rar") << QStringLiteral("application/vnd.rar") << QStringLiteral("Archive");
    QTest::newRow("iso") << QStringLiteral("application/x-cd-image") << QStringLiteral("Archive");
    QTest::newRow("rpm") << QStringLiteral("application/x-rpm") << QStringLiteral("Archive");
    QTest::newRow("deb") << QStringLiteral("application/vnd.debian.binary-package") << QStringLiteral("Archive");
    QTest::newRow("cbz is a comic, not a zip archive") << QStringLiteral("application/vnd.comicbook+zip") << QStringLiteral("Comic");
    QTest::newRow("cbr") << QStringLiteral("application/vnd.comicbook-rar") << QStringLiteral("Comic");
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

void ViewerResolverTest::testViewerForFile()
{
    QTemporaryDir dir;
    const QMimeType unknown = QMimeDatabase().mimeTypeForName(QStringLiteral("application/octet-stream"));

    QFile text(dir.filePath(QStringLiteral("NOTES")));
    QVERIFY(text.open(QIODevice::WriteOnly));
    text.write("plain words\n");
    text.close();
    QCOMPARE(ViewerResolver::viewerForFile(QFileInfo(text.fileName()), unknown), QStringLiteral("Text"));

    QFile empty(dir.filePath(QStringLiteral("EMPTY")));
    QVERIFY(empty.open(QIODevice::WriteOnly));
    empty.close();
    QCOMPARE(ViewerResolver::viewerForFile(QFileInfo(empty.fileName()), unknown), QStringLiteral("Info"));

    // Reading a FIFO would block forever; it must not even be opened.
    const QByteArray fifo = QFile::encodeName(dir.filePath(QStringLiteral("pipe")));
    QCOMPARE(::mkfifo(fifo.constData(), 0600), 0);
    QCOMPARE(ViewerResolver::viewerForFile(QFileInfo(QString::fromLocal8Bit(fifo)), unknown), QStringLiteral("Info"));
}

QTEST_GUILESS_MAIN(ViewerResolverTest)

#include "viewerresolvertest.moc"
