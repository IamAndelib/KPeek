// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include "filenavigator.h"
#include "naturalsort.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

namespace
{
/// What PeekController does, synchronously: the folder listing normally arrives from the worker pool.
void select(FileNavigator &nav, const QList<QUrl> &urls)
{
    nav.setSelection(urls);
    if (nav.siblingsNeeded()) {
        nav.setSiblings(FileNavigator::listSiblings(urls.first()));
    }
}
}

class FileNavigatorTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void testSingleFileBrowsesFolder();
    void testNaturalSortFoldersFirst();
    void testNaturalSortInCLocale();
    void testHiddenFileStillShown();
    void testMultiSelectionBrowsesOnlySelection();
    void testStepWraps();
    void testMatchesSelection();
    void testCaseOnlyDifference();
    void testListingRefreshesWhenFolderChanges();
    void testTrashedFile();

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
    select(nav, {urlFor(QStringLiteral("b.txt"))});
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

void FileNavigatorTest::testNaturalSortInCLocale()
{
    // Byte order would put "a10" first ('1' < '2') and "B" before "a" (uppercase first).
    const QLocale previous;
    QLocale::setDefault(QLocale::c());
    const QCollator collator = naturalCollator();
    QLocale::setDefault(previous);
    QVERIFY(collator.compare(QStringLiteral("a2"), QStringLiteral("a10")) < 0);
    QVERIFY(collator.compare(QStringLiteral("a"), QStringLiteral("B")) < 0);
}

void FileNavigatorTest::testHiddenFileStillShown()
{
    FileNavigator nav;
    select(nav, {urlFor(QStringLiteral(".hidden"))});
    QCOMPARE(nav.count(), 1);
    QCOMPARE(nav.current(), urlFor(QStringLiteral(".hidden")));
}

void FileNavigatorTest::testMultiSelectionBrowsesOnlySelection()
{
    const QList<QUrl> selection = {urlFor(QStringLiteral("b.txt")), urlFor(QStringLiteral("A2.txt"))};
    FileNavigator nav;
    select(nav, selection);
    QCOMPARE(nav.count(), selection.size());
    QCOMPARE(nav.current(), selection.first());
    nav.step(1);
    QCOMPARE(nav.current(), selection.last());
    nav.step(-1);
    QCOMPARE(nav.currentIndex(), 0);
}

void FileNavigatorTest::testStepWraps()
{
    FileNavigator nav;
    select(nav, {urlFor(QStringLiteral("b.txt"))});
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

    select(nav, {urlFor(QStringLiteral("b.txt"))});
    QVERIFY(nav.matchesSelection({urlFor(QStringLiteral("b.txt"))}));
    nav.step(1);
    // After browsing away, pressing the key on the newly shown file closes it, not the original.
    QVERIFY(!nav.matchesSelection({urlFor(QStringLiteral("b.txt"))}));
    QVERIFY(nav.matchesSelection({urlFor(QStringLiteral("zfolder"))}));

    const QList<QUrl> multi = {urlFor(QStringLiteral("b.txt")), urlFor(QStringLiteral("A2.txt"))};
    select(nav, multi);
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
    select(nav, {QUrl::fromLocalFile(first.fileName())});
    QCOMPARE(nav.count(), 1);

    QTest::qWait(1100); // folder modification times may only have one-second resolution
    QFile second(dir.filePath(QStringLiteral("b.txt")));
    QVERIFY(second.open(QIODevice::WriteOnly));
    second.close();

    select(nav, {QUrl::fromLocalFile(first.fileName())});
    QCOMPARE(nav.count(), 2);
}

void FileNavigatorTest::testTrashedFile()
{
    QTemporaryDir trash;
    QVERIFY(QDir(trash.path()).mkpath(QStringLiteral("files/Old Folder/inner")));
    QVERIFY(QFile(trash.filePath(QStringLiteral("files/photo 1.png"))).open(QIODevice::WriteOnly));
    QVERIFY(QFile(trash.filePath(QStringLiteral("files/Old Folder/inner/a.txt"))).open(QIODevice::WriteOnly));
    QVERIFY(QFile(trash.filePath(QStringLiteral("secret"))).open(QIODevice::WriteOnly));
    const auto map = [&](const QString &url) {
        return trashedFileUrl(QUrl(url), trash.path());
    };

    QCOMPARE(map(QStringLiteral("trash:/0-photo%201.png")), QUrl::fromLocalFile(trash.filePath(QStringLiteral("files/photo 1.png"))));
    QCOMPARE(map(QStringLiteral("trash:/0-Old%20Folder/inner/a.txt")), QUrl::fromLocalFile(trash.filePath(QStringLiteral("files/Old Folder/inner/a.txt"))));
    // Unknown, or anything trying to leave the trash's files folder, stays a trash URL (the info card).
    QCOMPARE(map(QStringLiteral("trash:/0-missing.png")), QUrl(QStringLiteral("trash:/0-missing.png")));
    QCOMPARE(map(QStringLiteral("trash:/0-../secret")), QUrl(QStringLiteral("trash:/0-../secret")));
    QCOMPARE(map(QStringLiteral("trash:/0-Old%20Folder/../../secret")), QUrl(QStringLiteral("trash:/0-Old%20Folder/../../secret")));
    QCOMPARE(map(QStringLiteral("trash:/")), QUrl(QStringLiteral("trash:/")));
    const QUrl local = QUrl::fromLocalFile(trash.filePath(QStringLiteral("secret")));
    QCOMPARE(trashedFileUrl(local, trash.path()), local);
}

QTEST_GUILESS_MAIN(FileNavigatorTest)

#include "filenavigatortest.moc"
