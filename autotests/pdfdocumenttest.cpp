// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include "pdfdocument.h"

#include <QTest>
#include <QUrl>

class PdfDocumentTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testCacheKeyNormalisesEncoding();
};

void PdfDocumentTest::testCacheKeyNormalisesEncoding()
{
    // The page renderer gets the key partly decoded by Qt; readLayout() gets it fully encoded.
    // Both must reach the same cached document, or the PDF is opened twice.
    const QString path = QStringLiteral("/home/me/my doc ü.pdf");
    const QString encoded = QStringLiteral("123/") + QString::fromLatin1(QUrl::toPercentEncoding(path));
    const QString partlyDecoded = QStringLiteral("123/%2Fhome%2Fme%2Fmy doc ü.pdf");
    QCOMPARE(PdfDocument::cacheKey(encoded), QStringLiteral("123/") + path);
    QCOMPARE(PdfDocument::cacheKey(partlyDecoded), QStringLiteral("123/") + path);
}

QTEST_GUILESS_MAIN(PdfDocumentTest)

#include "pdfdocumenttest.moc"
