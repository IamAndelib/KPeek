// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

// Runs the QML tests in autotests/qml. Linking Qt Widgets makes QUICK_TEST_MAIN use QApplication,
// which KDE's desktop style (and so its ScrollView) needs, matching the app.

#include <QQuickStyle>
#include <QtQuickTest>

class Setup : public QObject
{
    Q_OBJECT

public Q_SLOTS:
    void applicationAvailable()
    {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    }
};

QUICK_TEST_MAIN_WITH_SETUP(kpeek, Setup)

#include "qmltests.moc"
