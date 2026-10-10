// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include "archivemodel.h"

#include <archive.h>
#include <archive_entry.h>

#include <QFile>
#include <QTemporaryDir>
#include <QTest>

class ArchiveModelTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testOrder();
    void testCancelled();
    void testNotAnArchive();
    void testOtherFormats_data();
    void testOtherFormats();
    void testAbsurdPaths();
    void testEntryCap();
    void testIncomplete();

private:
    QString makeZip(const QStringList &files);
    QString makeArchive(const QString &name, int format, int filter, const QStringList &files);

    QTemporaryDir m_dir;
};

QString ArchiveModelTest::makeArchive(const QString &name, int format, int filter, const QStringList &files)
{
    // Writes one-byte files; folders only exist implicitly through the paths, as in many real zips.
    const QString path = m_dir.filePath(name);
    archive *writer = archive_write_new();
    archive_write_set_format(writer, format);
    archive_write_add_filter(writer, filter);
    if (archive_write_open_filename(writer, QFile::encodeName(path).constData()) != ARCHIVE_OK) {
        archive_write_free(writer);
        return {};
    }
    for (const QString &file : files) {
        archive_entry *entry = archive_entry_new();
        archive_entry_set_pathname_utf8(entry, file.toUtf8().constData());
        archive_entry_set_filetype(entry, AE_IFREG);
        archive_entry_set_perm(entry, 0644);
        archive_entry_set_size(entry, 1);
        archive_write_header(writer, entry);
        archive_write_data(writer, "x", 1);
        archive_entry_free(entry);
    }
    archive_write_close(writer);
    archive_write_free(writer);
    return path;
}

QString ArchiveModelTest::makeZip(const QStringList &files)
{
    return makeArchive(QStringLiteral("test.zip"), ARCHIVE_FORMAT_ZIP, ARCHIVE_FILTER_NONE, files);
}

void ArchiveModelTest::testOrder()
{
    // Names that collate equal ("A"/"a", "file01"/"file1") used to break std::sort's ordering rules.
    const QString zip = makeZip({QStringLiteral("file1"),
                                 QStringLiteral("A/z"),
                                 QStringLiteral("a/b"),
                                 QStringLiteral("A/a"),
                                 QStringLiteral("file01"),
                                 QStringLiteral("dir/sub/x"),
                                 QStringLiteral("dir/y")});
    QVERIFY(!zip.isEmpty());

    const ArchiveListing listing = ArchiveModel::list(zip);
    QVERIFY(!listing.failed);
    QStringList paths;
    for (const ArchiveEntry &entry : listing.entries) {
        paths.append(entry.path);
    }
    // Folders first, each directly above its contents; equal-collating names in a stable raw order.
    const QStringList expected = {QStringLiteral("A"),
                                  QStringLiteral("A/a"),
                                  QStringLiteral("A/z"),
                                  QStringLiteral("a"),
                                  QStringLiteral("a/b"),
                                  QStringLiteral("dir"),
                                  QStringLiteral("dir/sub"),
                                  QStringLiteral("dir/sub/x"),
                                  QStringLiteral("dir/y"),
                                  QStringLiteral("file01"),
                                  QStringLiteral("file1")};
    QCOMPARE(paths, expected);
    QCOMPARE(listing.fileCount, 7);
    QCOMPARE(listing.totalSize, 7);
}

void ArchiveModelTest::testCancelled()
{
    const QString zip = makeZip({QStringLiteral("a"), QStringLiteral("b")});
    const std::atomic_bool cancelled = true;
    QVERIFY(ArchiveModel::list(zip, &cancelled).entries.isEmpty());
}

void ArchiveModelTest::testNotAnArchive()
{
    const QString path = m_dir.filePath(QStringLiteral("broken.zip"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("not a zip");
    file.close();
    QVERIFY(ArchiveModel::list(path).failed);
}

void ArchiveModelTest::testOtherFormats_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<int>("format");
    QTest::addColumn<int>("filter");
    QTest::newRow("tar.zst") << QStringLiteral("t.tar.zst") << int(ARCHIVE_FORMAT_TAR_PAX_RESTRICTED) << int(ARCHIVE_FILTER_ZSTD);
    QTest::newRow("tar.xz") << QStringLiteral("t.tar.xz") << int(ARCHIVE_FORMAT_TAR_PAX_RESTRICTED) << int(ARCHIVE_FILTER_XZ);
    QTest::newRow("cpio") << QStringLiteral("t.cpio") << int(ARCHIVE_FORMAT_CPIO_POSIX) << int(ARCHIVE_FILTER_NONE);
    QTest::newRow("7z") << QStringLiteral("t.7z") << int(ARCHIVE_FORMAT_7ZIP) << int(ARCHIVE_FILTER_NONE);
}

void ArchiveModelTest::testOtherFormats()
{
    QFETCH(QString, name);
    QFETCH(int, format);
    QFETCH(int, filter);
    // A repeated path is listed once; the implicit folder "docs" is synthesised.
    const QString path = makeArchive(name, format, filter, {QStringLiteral("docs/readme.txt"), QStringLiteral("docs/readme.txt"), QStringLiteral("top.bin")});
    QVERIFY(!path.isEmpty());
    const ArchiveListing listing = ArchiveModel::list(path);
    QVERIFY(!listing.failed);
    QStringList paths;
    for (const ArchiveEntry &entry : listing.entries) {
        paths.append(entry.path);
    }
    QCOMPARE(paths, (QStringList{QStringLiteral("docs"), QStringLiteral("docs/readme.txt"), QStringLiteral("top.bin")}));
    QCOMPARE(listing.fileCount, 2);
}

void ArchiveModelTest::testAbsurdPaths()
{
    // Every parent folder is listed, so one path thousands of levels deep would cost gigabytes.
    // Such entries are skipped; the rest of the archive is listed normally.
    QString deep;
    for (int i = 0; i < 5000; ++i) {
        deep += QStringLiteral("a/");
    }
    const QString zip =
        makeArchive(QStringLiteral("deep.zip"), ARCHIVE_FORMAT_ZIP, ARCHIVE_FILTER_NONE, {deep + QStringLiteral("x"), QStringLiteral("ok.txt")});
    const ArchiveListing listing = ArchiveModel::list(zip);
    QVERIFY(!listing.failed);
    QCOMPARE(listing.entries.size(), 1);
    QCOMPARE(listing.entries.first().path, QStringLiteral("ok.txt"));
}

void ArchiveModelTest::testEntryCap()
{
    // The only memory guard against archives with millions of entries.
    QStringList files;
    for (int i = 0; i < ArchiveModel::MaxEntries + 5; ++i) {
        files.append(QStringLiteral("f%1").arg(i));
    }
    const ArchiveListing listing = ArchiveModel::list(makeArchive(QStringLiteral("many.zip"), ARCHIVE_FORMAT_ZIP, ARCHIVE_FILTER_NONE, files));
    QVERIFY(listing.truncated);
    QCOMPARE(listing.entries.size(), ArchiveModel::MaxEntries);
}

void ArchiveModelTest::testIncomplete()
{
    // A partial download lists what can be read and says the archive is incomplete. (A plain tar cut
    // at a block boundary is indistinguishable from a complete one; compressed streams are not.)
    QStringList files;
    for (int i = 0; i < 2000; ++i) {
        files.append(QStringLiteral("file%1.txt").arg(i));
    }
    const QString tar = makeArchive(QStringLiteral("cut.tar.gz"), ARCHIVE_FORMAT_TAR_PAX_RESTRICTED, ARCHIVE_FILTER_GZIP, files);
    QFile file(tar);
    QVERIFY(file.resize(file.size() / 2));
    const ArchiveListing listing = ArchiveModel::list(tar);
    QVERIFY(!listing.failed);
    QVERIFY(listing.incomplete);
    QVERIFY(listing.fileCount > 0 && listing.fileCount < 2000);
}

QTEST_GUILESS_MAIN(ArchiveModelTest)

#include "archivemodeltest.moc"
