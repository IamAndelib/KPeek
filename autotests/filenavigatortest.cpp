// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibsriz@proton.me>

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

QTEST_GUILESS_MAIN(FileNavigatorTest)

#include "filenavigatortest.moc"
