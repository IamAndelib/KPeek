// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include "filenavigator.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

class FileNavigatorTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void testSingleFileBrowsesFolder();
    void testNaturalSortFoldersFirst();
    void testHiddenFileStillShown();
    void testMultiSelectionBrowsesOnlySelection();
    void testStepWraps();
    void testMatchesSelection();
    void testCaseOnlyDifference();
    void testListingRefreshesWhenFolderChanges();

private:
    QUrl urlFor(const QString &name) const;

    QTemporaryDir m_dir;
};

QUrl FileNavigatorTest::urlFor(const QString &name) const
{
    return QUrl::fromLocalFile(m_dir.filePath(name));
}

void FileNavigatorTest::init()
{
    QVERIFY(m_dir.isValid());
    const QStringList files = {QStringLiteral("b.txt"), QStringLiteral("a10.txt"), QStringLiteral("A2.txt"), QStringLiteral(".hidden")};
    for (const QString &name : files) {
        QFile file(m_dir.filePath(name));
        QVERIFY(file.open(QIODevice::WriteOnly));
    }
    QVERIFY(QDir(m_dir.path()).mkpath(QStringLiteral("zfolder")));
}

void FileNavigatorTest::testSingleFileBrowsesFolder()
{
    FileNavigator nav;
    nav.setSelection({urlFor(QStringLiteral("b.txt"))});
    QCOMPARE(nav.count(), 4); // zfolder, A2, a10, b; .hidden excluded
    QCOMPARE(nav.current(), urlFor(QStringLiteral("b.txt")));
    QCOMPARE(nav.currentIndex(), 3);
}

void FileNavigatorTest::testNaturalSortFoldersFirst()
{
    const QList<QUrl> expected = {urlFor(QStringLiteral("zfolder")),
                                  urlFor(QStringLiteral("A2.txt")),
                                  urlFor(QStringLiteral("a10.txt")),
                                  urlFor(QStringLiteral("b.txt"))};
    QCOMPARE(FileNavigator::siblings(urlFor(QStringLiteral("b.txt"))), expected);
}

void FileNavigatorTest::testHiddenFileStillShown()
{
    FileNavigator nav;
    nav.setSelection({urlFor(QStringLiteral(".hidden"))});
    QCOMPARE(nav.count(), 1);
    QCOMPARE(nav.current(), urlFor(QStringLiteral(".hidden")));
}

void FileNavigatorTest::testMultiSelectionBrowsesOnlySelection()
{
    const QList<QUrl> selection = {urlFor(QStringLiteral("b.txt")), urlFor(QStringLiteral("A2.txt"))};
    FileNavigator nav;
    nav.setSelection(selection);
    QCOMPARE(nav.items(), selection);
    QCOMPARE(nav.currentIndex(), 0);
}

void FileNavigatorTest::testStepWraps()
{
    FileNavigator nav;
    nav.setSelection({urlFor(QStringLiteral("b.txt"))});
    nav.step(1);
    QCOMPARE(nav.current(), urlFor(QStringLiteral("zfolder")));
    nav.step(-1);
    QCOMPARE(nav.current(), urlFor(QStringLiteral("b.txt")));
    nav.step(-5);
    QCOMPARE(nav.current(), urlFor(QStringLiteral("a10.txt")));
}

void FileNavigatorTest::testMatchesSelection()
{
    FileNavigator nav;
    QVERIFY(!nav.matchesSelection({urlFor(QStringLiteral("b.txt"))}));

    nav.setSelection({urlFor(QStringLiteral("b.txt"))});
    QVERIFY(nav.matchesSelection({urlFor(QStringLiteral("b.txt"))}));
    nav.step(1);
    // After browsing away, pressing the key on the newly shown file closes it, not the original.
    QVERIFY(!nav.matchesSelection({urlFor(QStringLiteral("b.txt"))}));
    QVERIFY(nav.matchesSelection({urlFor(QStringLiteral("zfolder"))}));

    const QList<QUrl> multi = {urlFor(QStringLiteral("b.txt")), urlFor(QStringLiteral("A2.txt"))};
    nav.setSelection(multi);
    QVERIFY(nav.matchesSelection(multi));
}

void FileNavigatorTest::testCaseOnlyDifference()
{
    // Names equal under case-insensitive collation still get a fixed, strict order.
    QTemporaryDir dir;
    for (const char *name : {"b.txt", "B.txt"}) {
        QFile file(dir.filePath(QString::fromLatin1(name)));
        QVERIFY(file.open(QIODevice::WriteOnly));
    }
    const QList<QUrl> siblings = FileNavigator::siblings(QUrl::fromLocalFile(dir.filePath(QStringLiteral("b.txt"))));
    QCOMPARE(siblings, (QList<QUrl>{QUrl::fromLocalFile(dir.filePath(QStringLiteral("B.txt"))), QUrl::fromLocalFile(dir.filePath(QStringLiteral("b.txt")))}));
}

void FileNavigatorTest::testListingRefreshesWhenFolderChanges()
{
    // The folder listing is cached between invocations, but a new file must show up.
    QTemporaryDir dir;
    QFile first(dir.filePath(QStringLiteral("a.txt")));
    QVERIFY(first.open(QIODevice::WriteOnly));
    first.close();

    FileNavigator nav;
    nav.setSelection({QUrl::fromLocalFile(first.fileName())});
    QCOMPARE(nav.count(), 1);

    QTest::qWait(1100); // folder modification times may only have one-second resolution
    QFile second(dir.filePath(QStringLiteral("b.txt")));
    QVERIFY(second.open(QIODevice::WriteOnly));
    second.close();

    nav.setSelection({QUrl::fromLocalFile(first.fileName())});
    QCOMPARE(nav.count(), 2);
}

QTEST_GUILESS_MAIN(FileNavigatorTest)

#include "filenavigatortest.moc"
