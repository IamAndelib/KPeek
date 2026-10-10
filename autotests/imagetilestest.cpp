// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include "imagetiles.h"

#include <QImageReader>
#include <QImageWriter>
#include <QPainter>
#include <QTemporaryDir>
#include <QTest>

// A tile of a quadrant must have the colour that quadrant shows on screen, in every EXIF orientation.
class ImageTilesTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testTileColours_data();
    void testTileColours();
    void testMapToFileRoundTrip();
};

namespace
{
bool close(QColor a, QColor b)
{
    return std::abs(a.red() - b.red()) < 24 && std::abs(a.green() - b.green()) < 24 && std::abs(a.blue() - b.blue()) < 24;
}
}

void ImageTilesTest::testTileColours_data()
{
    QTest::addColumn<QImageIOHandler::Transformations>("transformation");
    QTest::newRow("none") << QImageIOHandler::Transformations(QImageIOHandler::TransformationNone);
    QTest::newRow("mirror") << QImageIOHandler::Transformations(QImageIOHandler::TransformationMirror);
    QTest::newRow("flip") << QImageIOHandler::Transformations(QImageIOHandler::TransformationFlip);
    QTest::newRow("rotate180") << QImageIOHandler::Transformations(QImageIOHandler::TransformationRotate180);
    QTest::newRow("rotate90") << QImageIOHandler::Transformations(QImageIOHandler::TransformationRotate90);
    QTest::newRow("mirror+rotate90") << QImageIOHandler::Transformations(QImageIOHandler::TransformationMirrorAndRotate90);
    QTest::newRow("flip+rotate90") << QImageIOHandler::Transformations(QImageIOHandler::TransformationFlipAndRotate90);
    QTest::newRow("rotate270") << QImageIOHandler::Transformations(QImageIOHandler::TransformationRotate270);
}

void ImageTilesTest::testTileColours()
{
    QFETCH(QImageIOHandler::Transformations, transformation);

    // 160×96 file with four solid quadrants; non-square so rotations are visible in the size.
    QImage source(160, 96, QImage::Format_RGB32);
    QPainter painter(&source);
    painter.fillRect(0, 0, 80, 48, Qt::red);
    painter.fillRect(80, 0, 80, 48, Qt::green);
    painter.fillRect(0, 48, 80, 48, Qt::blue);
    painter.fillRect(80, 48, 80, 48, Qt::yellow);
    painter.end();

    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("photo.jpg"));
    QImageWriter writer(path);
    writer.setQuality(100);
    writer.setTransformation(transformation);
    QVERIFY(writer.write(source));

    // What the user sees: the whole image with its orientation applied.
    QImageReader reader(path);
    reader.setAutoTransform(true);
    const QImage displayed = reader.read();
    QVERIFY(!displayed.isNull());

    for (const qreal scale : {1.0, 0.5}) {
        const QSize scaledSize = (QSizeF(displayed.size()) * scale).toSize();
        const int halfW = scaledSize.width() / 2;
        const int halfH = scaledSize.height() / 2;
        const int inset = 4;
        for (const QPoint quadrant : {QPoint(0, 0), QPoint(1, 0), QPoint(0, 1), QPoint(1, 1)}) {
            const QRect clip(quadrant.x() * halfW + inset, quadrant.y() * halfH + inset, halfW - 2 * inset, halfH - 2 * inset);
            const QImage tile = ImageTiles::decode(path, scaledSize, clip);
            QCOMPARE(tile.size(), clip.size());
            const QColor expected = displayed.pixelColor((QPointF(clip.center()) / scale).toPoint());
            const QColor actual = tile.pixelColor(tile.rect().center());
            QVERIFY2(close(actual, expected),
                     qPrintable(QStringLiteral("scale %1, quadrant %2,%3: got %4, expected %5")
                                    .arg(scale)
                                    .arg(quadrant.x())
                                    .arg(quadrant.y())
                                    .arg(actual.name(), expected.name())));
        }
    }
}

void ImageTilesTest::testMapToFileRoundTrip()
{
    // Rotated by 90°, the top-left 10×20 of a 30×50 displayed image is the bottom-left 20×10 of the 50×30 file.
    QCOMPARE(ImageTiles::mapToFile(QRect(0, 0, 10, 20), QSize(30, 50), QImageIOHandler::TransformationRotate90), QRect(0, 20, 20, 10));
    QCOMPARE(ImageTiles::mapToFile(QRect(0, 0, 10, 20), QSize(30, 50), QImageIOHandler::TransformationNone), QRect(0, 0, 10, 20));
    QCOMPARE(ImageTiles::mapToFile(QRect(0, 0, 10, 20), QSize(30, 50), QImageIOHandler::TransformationMirror), QRect(20, 0, 10, 20));
}

QTEST_GUILESS_MAIN(ImageTilesTest)

#include "imagetilestest.moc"
