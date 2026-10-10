// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include "comicbook.h"

#include <QBuffer>
#include <QFile>
#include <QImage>
#include <QTemporaryDir>
#include <QTest>

#include <archive.h>
#include <archive_entry.h>

class ComicBookTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testLayout();
    void testReadPage();
    void testNoPages();
    void testDotSlashNames();

private:
    QTemporaryDir m_dir;
    QString m_cbz;
};

namespace
{
QByteArray png(int width, int height)
{
    QImage image(width, height, QImage::Format_RGB32);
    image.fill(Qt::darkCyan);
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return data;
}

void writeZip(const QString &path, const QList<QPair<QString, QByteArray>> &files)
{
    archive *writer = archive_write_new();
    archive_write_set_format_zip(writer);
    archive_write_open_filename(writer, QFile::encodeName(path).constData());
    for (const auto &[name, data] : files) {
        archive_entry *entry = archive_entry_new();
        archive_entry_set_pathname_utf8(entry, name.toUtf8().constData());
        archive_entry_set_filetype(entry, AE_IFREG);
        archive_entry_set_perm(entry, 0644);
        archive_entry_set_size(entry, data.size());
        archive_write_header(writer, entry);
        archive_write_data(writer, data.constData(), data.size());
        archive_entry_free(entry);
    }
    archive_write_close(writer);
    archive_write_free(writer);
}
}

void ComicBookTest::initTestCase()
{
    QVERIFY(m_dir.isValid());
    m_cbz = m_dir.filePath(QStringLiteral("issue.cbz"));
    writeZip(m_cbz,
             {{QStringLiteral("page10.png"), png(100, 150)},
              {QStringLiteral("page2.png"), png(200, 100)}, // a landscape spread
              {QStringLiteral("page1.png"), png(100, 150)},
              {QStringLiteral("ComicInfo.xml"), QByteArray("<ComicInfo/>")},
              {QStringLiteral("__MACOSX/._page1.png"), QByteArray("fork")},
              {QStringLiteral(".hidden.png"), png(10, 10)}});
}

void ComicBookTest::testLayout()
{
    const ComicBook::Layout layout = ComicBook::readLayout(m_cbz);
    QVERIFY(!layout.failed);
    // Natural order; metadata, resource forks and hidden files are not pages.
    QCOMPARE(layout.entries, (QStringList{QStringLiteral("page1.png"), QStringLiteral("page2.png"), QStringLiteral("page10.png")}));
    QCOMPARE(layout.sizes, (QList<QSizeF>{QSizeF(100, 150), QSizeF(200, 100), QSizeF(100, 150)}));
}

void ComicBookTest::testReadPage()
{
    // Decoded at the requested width, keeping the aspect ratio, never larger than the page itself.
    QCOMPARE(ComicBook::readPage(m_cbz, QStringLiteral("page2.png"), 50).size(), QSize(50, 25));
    QCOMPARE(ComicBook::readPage(m_cbz, QStringLiteral("page2.png"), 4000).size(), QSize(200, 100));
    QVERIFY(ComicBook::readPage(m_cbz, QStringLiteral("missing.png"), 50).isNull());
}

void ComicBookTest::testNoPages()
{
    const QString path = m_dir.filePath(QStringLiteral("empty.cbz"));
    writeZip(path, {{QStringLiteral("readme.txt"), QByteArray("no pictures")}});
    QVERIFY(ComicBook::readLayout(path).failed);
}

void ComicBookTest::testDotSlashNames()
{
    // "tar -C pages ." stores "./page1.png": that's a page, not a hidden file.
    const QString path = m_dir.filePath(QStringLiteral("dotslash.cbz"));
    writeZip(path, {{QStringLiteral("./page2.png"), png(10, 10)}, {QStringLiteral("./page1.png"), png(10, 10)}});
    const ComicBook::Layout layout = ComicBook::readLayout(path);
    QCOMPARE(layout.entries, (QStringList{QStringLiteral("page1.png"), QStringLiteral("page2.png")}));
    QCOMPARE(ComicBook::readPage(path, layout.entries.first(), 10).size(), QSize(10, 10));
}

QTEST_GUILESS_MAIN(ComicBookTest)

#include "comicbooktest.moc"
