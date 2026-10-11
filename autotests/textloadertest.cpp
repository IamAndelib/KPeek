// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include "textloader.h"

#include <QImage>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <sys/stat.h>

class TextLoaderTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testDecode_data();
    void testDecode();
    void testCutCharacter();
    void testLargeFile();
    void testUnreadable();
    void testBreakLongLines();
    void testMarkdownImages();
};

void TextLoaderTest::testDecode_data()
{
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QString>("text");

    QTest::newRow("utf-8") << QByteArray("caf\xc3\xa9") << QStringLiteral("café");
    QTest::newRow("latin-1 fallback") << QByteArray("caf\xe9") << QStringLiteral("café");
    QTest::newRow("latin-1 mid-text") << QByteArray("na\xefve") << QStringLiteral("naïve");
    QTest::newRow("utf-16 bom") << QByteArray("\xff\xfeh\0i\0", 6) << QStringLiteral("hi");
}

void TextLoaderTest::testDecode()
{
    QFETCH(QByteArray, data);
    QFETCH(QString, text);
    QCOMPARE(TextLoader::decode(data), text);
}

void TextLoaderTest::testCutCharacter()
{
    // A UTF-8 file cut at the size limit inside "é" must stay UTF-8, not fall back to Latin-1.
    QCOMPARE(TextLoader::decode(QByteArray("caf\xc3\xa9 caf\xc3"), false), QStringLiteral("café caf"));
}

void TextLoaderTest::testLargeFile()
{
    QTemporaryDir dir;
    QFile file(dir.filePath(QStringLiteral("big.txt")));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(QByteArray(TextLoader::MaxBytes + 10, 'x'));
    file.close();

    TextLoader loader;
    loader.setUrl(QUrl::fromLocalFile(file.fileName()));
    QVERIFY(loader.truncated());
    // One long line: broken up so it can be laid out, nothing lost.
    QCOMPARE(loader.text().size(), TextLoader::MaxBytes + TextLoader::MaxBytes / TextLoader::MaxLineChars);
    QCOMPARE(QString(loader.text()).remove(QLatin1Char('\n')).size(), TextLoader::MaxBytes);
    QVERIFY(!loader.highlight());
    QVERIFY(!loader.failed());
}

void TextLoaderTest::testUnreadable()
{
    TextLoader loader;
    loader.setUrl(QUrl::fromLocalFile(QStringLiteral("/nonexistent/kpeek-test")));
    QVERIFY(loader.failed());
    QVERIFY(loader.text().isEmpty());
}

void TextLoaderTest::testBreakLongLines()
{
    const qsizetype n = TextLoader::MaxLineChars;
    const QString shortLines = QStringLiteral("a\nbb\n");
    QCOMPARE(TextLoader::breakLongLines(shortLines), shortLines);
    QCOMPARE(TextLoader::breakLongLines(QString(n, u'x')), QString(n, u'x'));
    QCOMPARE(TextLoader::breakLongLines(QString(n + 1, u'x')), QString(n, u'x') + u'\n' + u'x');
    // Counting restarts after every line break, existing or inserted.
    const QString mixed = QStringLiteral("ab\n") + QString(2 * n + 3, u'y');
    QCOMPARE(TextLoader::breakLongLines(mixed), QStringLiteral("ab\n") + QString(n, u'y') + u'\n' + QString(n, u'y') + u'\n' + QStringLiteral("yyy"));
}

void TextLoaderTest::testMarkdownImages()
{
    QTemporaryDir dir;
    QImage(4, 4, QImage::Format_RGB32).save(dir.filePath(QStringLiteral("pic.png")));
    QVERIFY(mkfifo(QFile::encodeName(dir.filePath(QStringLiteral("pipe.png"))).constData(), 0600) == 0);
    const QUrl base = QUrl::fromLocalFile(dir.filePath(QStringLiteral("README.md")));

    const QString html =
        TextLoader::markdownToHtml(QStringLiteral("<p align=\"center\">\n  <img src=\"pic.png\" width=\"99999\">\n</p>\n\n# Title\n\n"
                                                  "![a](pic.png) ![b](/dev/zero) ![c](pipe.png) ![d](missing.png) "
                                                  "![e](https://example.com/x.png)\n\nPress <kbd>Space</kbd>. Inline <img src=\"/dev/zero\"> "
                                                  "image.\n\n<table background=\"/dev/zero\"><tr><td background=\"/dev/zero\">t</td></tr></table>\n\n"
                                                  "<div style=\"background-image: url(/dev/zero)\">d</div>\n\n```\nCode <br> stays\n```\n\n"
                                                  "<link rel=\"stylesheet\" href=\"/dev/zero\">\n\nThe end."),
                                   base);
    // Only the real, local picture is still an image (as a centred logo and inline), by its absolute path,
    // at its own size.
    QCOMPARE(html.count(QLatin1String("<img ")), 2);
    QCOMPARE(html.count(QLatin1String("<img src=\"") + QUrl::fromLocalFile(dir.filePath(QStringLiteral("pic.png"))).toString() + QLatin1Char('"')), 2);
    QVERIFY(!html.contains(QLatin1String("99999")));
    QVERIFY(!html.contains(QLatin1String("/dev/zero"))); // nor as a background or a stylesheet
    QVERIFY(!html.contains(QLatin1String("pipe.png")));
    QVERIFY(!html.contains(QLatin1String("example.com")));
    // Nothing after an HTML image is lost (Qt's importer used to drop it), harmless HTML renders, code is untouched.
    QVERIFY(html.contains(QLatin1String("Title")) && html.contains(QLatin1String("image.")) && html.contains(QLatin1String("The end.")));
    QVERIFY(html.contains(QLatin1String("Space")) && !html.contains(QLatin1String("&lt;kbd")));
    QVERIFY(html.contains(QLatin1String(">t<")) && html.contains(QLatin1String(">d<")));
    QVERIFY(html.contains(QLatin1String("Code &lt;br&gt; stays")));
    QVERIFY(!html.contains(QLatin1String("<body style"))); // the viewer's font and zoom apply
}

QTEST_MAIN(TextLoaderTest)

#include "textloadertest.moc"
