// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include "thumbnailcache.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QTemporaryDir>
#include <QTest>

class ThumbnailCacheTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void testFileName();
    void testNoThumbnail();
    void testPrefersLargestUpToDate();
    void testRejectsStale();

private:
    void writeThumbnail(const QString &size, qint64 mtime);

    QTemporaryDir m_dir;
    QString m_file;
    QString m_cache;
};

void ThumbnailCacheTest::init()
{
    m_file = m_dir.filePath(QStringLiteral("my photo.jpg"));
    QFile file(m_file);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("x");
    file.close();
    m_cache = m_dir.filePath(QStringLiteral("thumbnails"));
    QDir(m_cache).removeRecursively();
}

void ThumbnailCacheTest::writeThumbnail(const QString &size, qint64 mtime)
{
    QVERIFY(QDir().mkpath(m_cache + QLatin1Char('/') + size));
    // Like Dolphin's thumbnails: a tEXt chunk "Thumb::MTime" (set on the QImage; QImageWriter::setText would mangle the key).
    QImage image(8, 8, QImage::Format_RGB32);
    image.fill(Qt::gray);
    image.setText(QStringLiteral("Thumb::MTime"), QString::number(mtime));
    QVERIFY(image.save(m_cache + QLatin1Char('/') + size + QLatin1Char('/') + ThumbnailCache::fileNameFor(m_file), "PNG"));
}

void ThumbnailCacheTest::testFileName()
{
    // Per the freedesktop spec: MD5 of the percent-encoded file:// URI.
    const QByteArray uri = "file://" + QUrl::toPercentEncoding(m_file, "/");
    QCOMPARE(ThumbnailCache::fileNameFor(m_file), QString::fromLatin1(QCryptographicHash::hash(uri, QCryptographicHash::Md5).toHex()) + QStringLiteral(".png"));
}

void ThumbnailCacheTest::testNoThumbnail()
{
    QVERIFY(ThumbnailCache::find(m_file, m_cache).isEmpty());
}

void ThumbnailCacheTest::testPrefersLargestUpToDate()
{
    const qint64 mtime = QFileInfo(m_file).lastModified().toSecsSinceEpoch();
    writeThumbnail(QStringLiteral("large"), mtime);
    writeThumbnail(QStringLiteral("x-large"), mtime);
    QCOMPARE(ThumbnailCache::find(m_file, m_cache).toLocalFile(), m_cache + QStringLiteral("/x-large/") + ThumbnailCache::fileNameFor(m_file));
}

void ThumbnailCacheTest::testRejectsStale()
{
    const qint64 mtime = QFileInfo(m_file).lastModified().toSecsSinceEpoch();
    writeThumbnail(QStringLiteral("xx-large"), mtime - 100); // made before the file last changed
    writeThumbnail(QStringLiteral("normal"), mtime);
    QCOMPARE(ThumbnailCache::find(m_file, m_cache).toLocalFile(), m_cache + QStringLiteral("/normal/") + ThumbnailCache::fileNameFor(m_file));
}

QTEST_GUILESS_MAIN(ThumbnailCacheTest)

#include "thumbnailcachetest.moc"
