// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include <QPointingDevice>
#include <QQuickItem>
#include <QQuickView>
#include <QTest>
#include <QWheelEvent>

// Ctrl+wheel must zoom whatever kind of device Qt says the wheel belongs to. On Wayland, Qt reports a
// real mouse wheel as coming from a "touchpad"-typed device, which QtQuickTest's mouseWheel() can't simulate.
class ViewerInputTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testCtrlWheelZooms_data();
    void testCtrlWheelZooms();
};

void ViewerInputTest::testCtrlWheelZooms_data()
{
    QTest::addColumn<QInputDevice::DeviceType>("deviceType");
    QTest::addColumn<QPointingDevice::PointerType>("pointerType");

    QTest::newRow("mouse") << QInputDevice::DeviceType::Mouse << QPointingDevice::PointerType::Generic;
    QTest::newRow("wayland wheel (touchpad, finger)") << QInputDevice::DeviceType::TouchPad << QPointingDevice::PointerType::Finger;
}

void ViewerInputTest::testCtrlWheelZooms()
{
    QFETCH(QInputDevice::DeviceType, deviceType);
    QFETCH(QPointingDevice::PointerType, pointerType);

    QQuickView view;
    view.setSource(QUrl::fromLocalFile(QStringLiteral(KPEEK_TEST_DATA "/viewerinput.qml")));
    QVERIFY2(view.status() == QQuickView::Ready, qPrintable(view.errors().isEmpty() ? QString() : view.errors().first().toString()));
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QObject *input = view.rootObject()->findChild<QObject *>(QStringLiteral("input"));
    QVERIFY(input);
    QCOMPARE(input->property("zoom").toReal(), 1.0);

    QPointingDevice
        device(QStringLiteral("test wheel"), 4242, deviceType, pointerType, QInputDevice::Capability::Position | QInputDevice::Capability::Scroll, 1, 0);
    const QPointF pos(view.width() / 2.0, view.height() / 2.0);
    QWheelEvent event(pos,
                      view.mapToGlobal(pos),
                      QPoint(),
                      QPoint(0, 120),
                      Qt::NoButton,
                      Qt::ControlModifier,
                      Qt::NoScrollPhase,
                      false,
                      Qt::MouseEventNotSynthesized,
                      &device);
    QGuiApplication::sendEvent(&view, &event);

    QVERIFY2(input->property("zoom").toReal() > 1.0, "Ctrl+wheel up should zoom in");
}

QTEST_MAIN(ViewerInputTest)

#include "viewerinputtest.moc"
