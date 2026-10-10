// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

import QtQuick
import "../../src/qml"

// Fixture for viewerinputtest.cpp: a zoomable viewer under ViewerInput.
Item {
    width: 400
    height: 300

    Item {
        id: viewer
        anchors.fill: parent
        property real zoom: 1.0
        readonly property Flickable scrollTarget: null
    }

    ViewerInput {
        objectName: "input"
        anchors.fill: parent
        viewer: viewer
    }
}
