// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

import QtQuick
import QtQuick.Controls as QQC2
import QtTest
import "../../src/qml"

// Zoom and scrolling over a viewer built like the real text viewer: a styled ScrollView.
Item {
    id: root
    width: 400
    height: 300

    Item {
        id: viewer
        anchors.fill: parent
        property real zoom: 1.0
        readonly property Flickable scrollTarget: scroll.contentItem as Flickable

        QQC2.ScrollView {
            id: scroll
            anchors.fill: parent
            QQC2.TextArea {
                text: Array(400).fill("line").join("\n")
                readOnly: true
            }
        }
    }

    ViewerInput {
        id: input
        anchors.fill: parent
        viewer: viewer
    }

    TestCase {
        name: "ViewerInput"
        when: windowShown

        function init() {
            input.zoom = 1.0;
            viewer.scrollTarget.contentY = 0;
        }

        function test_ctrlWheelZooms() {
            mouseWheel(root, 200, 150, 0, 120, Qt.NoButton, Qt.ControlModifier);
            verify(input.zoom > 1.0, "Ctrl+wheel up should zoom in; zoom = " + input.zoom);
            mouseWheel(root, 200, 150, 0, -240, Qt.NoButton, Qt.ControlModifier);
            verify(input.zoom < 1.0, "Ctrl+wheel down should zoom out; zoom = " + input.zoom);
            compare(viewer.scrollTarget.contentY, 0, "Ctrl+wheel must not scroll");
        }

        function test_plainWheelScrolls() {
            mouseWheel(root, 200, 150, 0, -120, Qt.NoButton, Qt.NoModifier);
            tryVerify(() => viewer.scrollTarget.contentY > 0, 1000, "the plain wheel should scroll");
            compare(input.zoom, 1.0);
        }

        function test_zoomLimits() {
            input.zoomBy(100);
            compare(input.zoom, input.maxZoom);
            input.zoomBy(-100);
            compare(input.zoom, input.minZoom);
        }
    }
}
