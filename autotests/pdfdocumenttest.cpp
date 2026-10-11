// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include "pdfdocument.h"
#include "systeminfo.h"

#include <QPainter>
#include <QPdfWriter>
#include <QQuickImageResponse>
#include <QQuickTextureFactory>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

class PdfDocumentTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testCacheKeyNormalisesEncoding();
    void testLayout();
    void testRenderStaysWithinBudget();
    void testNotAPdf();

private:
    QString key(const QString &path) const
    {
        return QStringLiteral("1/") + QString::fromLatin1(QUrl::toPercentEncoding(path));
    }
    QString writePdf(const QString &name, QSizeF points, int pages);

    QTemporaryDir m_dir;
};

QString PdfDocumentTest::writePdf(const QString &name, QSizeF points, int pages)
{
    const QString path = m_dir.filePath(name);
    QPdfWriter writer(path);
    writer.setPageSize(QPageSize(points, QPageSize::Point));
    QPainter painter(&writer);
    for (int i = 0; i < pages; ++i) {
        if (i > 0) {
            writer.newPage();
        }
        painter.drawText(QPointF(20, 20), QString::number(i + 1));
    }
    return path;
}

void PdfDocumentTest::testLayout()
{
    const PdfDocument::Layout layout = PdfDocument::readLayout(key(writePdf(QStringLiteral("two.pdf"), QSizeF(300, 400), 2)));
    QVERIFY(!layout.failed);
    QCOMPARE(layout.pageSizes.size(), 2);
}

void PdfDocumentTest::testRenderStaysWithinBudget()
{
    // However wide the request, a hostile page shape (2 × 200 inches) never renders past the pixel
    // budget: a multi-GB image would take down the process. KPEEK_LOW_MEMORY=1 is set for
    // this test, so the budget is 24 MP.
    QVERIFY(isLowMemorySystem());
    const QString path = writePdf(QStringLiteral("strip.pdf"), QSizeF(144, 14400), 1);
    PdfPageProvider provider;
    QQuickImageResponse *response = provider.requestImageResponse(QStringLiteral("0/") + key(path), QSize(4096, 0));
    QSignalSpy finished(response, &QQuickImageResponse::finished);
    QVERIFY(finished.wait(10000));
    std::unique_ptr<QQuickTextureFactory> factory(response->textureFactory());
    const QImage image = factory->image();
    QVERIFY(!image.isNull());
    QVERIFY(qint64(image.width()) * image.height() <= maxPagePixels() * 1.01);
    delete response;
}

void PdfDocumentTest::testNotAPdf()
{
    const QString path = m_dir.filePath(QStringLiteral("fake.pdf"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("%PDF-1.4\nnot really");
    file.close();
    const PdfDocument::Layout layout = PdfDocument::readLayout(key(path));
    QVERIFY(layout.failed); // a message, not an empty viewer
    QVERIFY(!layout.locked);
}

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

QTEST_MAIN(PdfDocumentTest)

#include "pdfdocumenttest.moc"
