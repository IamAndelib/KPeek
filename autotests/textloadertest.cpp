// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include "textloader.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

class TextLoaderTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testDecode_data();
    void testDecode();
    void testCutCharacter();
    void testLargeFile();
    void testUnreadable();
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
    QCOMPARE(loader.text().size(), TextLoader::MaxBytes);
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

QTEST_GUILESS_MAIN(TextLoaderTest)

#include "textloadertest.moc"
