// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include "archivemodel.h"

#include <KZip>

#include <QTemporaryDir>
#include <QTest>

class ArchiveModelTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testOrder();
    void testCancelled();
    void testNotAnArchive();

private:
    QString makeZip(const QStringList &files);

    QTemporaryDir m_dir;
};

QString ArchiveModelTest::makeZip(const QStringList &files)
{
    const QString path = m_dir.filePath(QStringLiteral("test.zip"));
    KZip zip(path);
    if (!zip.open(QIODevice::WriteOnly)) {
        return {};
    }
    for (const QString &file : files) {
        zip.writeFile(file, QByteArray("x"));
    }
    zip.close();
    return path;
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
    QVERIFY(ArchiveModel::list(zip, cancelled).entries.isEmpty());
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

QTEST_GUILESS_MAIN(ArchiveModelTest)

#include "archivemodeltest.moc"
