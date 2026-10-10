// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

import QtQuick
import org.kde.kirigami as Kirigami

/**
 * Wheel, drag and key handling shared by all viewers. It sits on top of the viewer.
 *
 * A viewer takes part by exposing `scrollTarget` (its Flickable, or null when it has
 * nothing to scroll) and, if zooming means something for it, a `zoom` property.
 *  - Ctrl+wheel zooms. The plain wheel isn't touched: it scrolls the viewer natively.
 *  - Ctrl+left-drag pans the content.
 *  - Up/Down/PgUp/PgDn/Home/End scroll.
 */
Item {
    id: input

    property Item viewer
    property real zoom: 1.0
    readonly property Flickable flickable: viewer?.scrollTarget ?? null

    readonly property bool zoomable: viewer !== null && "zoom" in viewer
    readonly property real minZoom: 0.25
    readonly property real maxZoom: 4.0

    function zoomBy(steps) {
        zoom = Math.min(maxZoom, Math.max(minZoom, zoom * Math.pow(1.25, steps)));
    }

    function scrollTo(x, y) {
        const f = flickable;
        f.contentX = Math.max(f.originX, Math.min(x, f.originX + Math.max(0, f.contentWidth - f.width)));
        f.contentY = Math.max(f.originY, Math.min(y, f.originY + Math.max(0, f.contentHeight - f.height)));
    }

    function canScroll() {
        return flickable !== null && (flickable.contentHeight > flickable.height + 1 || flickable.contentWidth > flickable.width + 1);
    }

    function handleKey(event) {
        if (!flickable) {
            return;
        }
        const line = Kirigami.Units.gridUnit * 3;
        const page = flickable.height * 0.9;
        let y = flickable.contentY;
        switch (event.key) {
        case Qt.Key_Up: y -= line; break;
        case Qt.Key_Down: y += line; break;
        case Qt.Key_PageUp: y -= page; break;
        case Qt.Key_PageDown: y += page; break;
        case Qt.Key_Home: y = flickable.originY; break;
        case Qt.Key_End: y = flickable.originY + flickable.contentHeight; break;
        default: return;
        }
        scrollTo(flickable.contentX, y);
        event.accepted = true;
    }

    WheelHandler {
        target: null
        acceptedModifiers: Qt.ControlModifier
        enabled: input.zoomable
        // 120 per mouse wheel notch; touchpads send smaller steps for smooth zooming.
        onWheel: event => input.zoomBy(event.angleDelta.y / 120)
    }

    DragHandler {
        id: panHandler

        property point startContent

        target: null
        acceptedModifiers: Qt.ControlModifier
        enabled: input.canScroll()
        cursorShape: active ? Qt.ClosedHandCursor : Qt.ArrowCursor
        onActiveChanged: {
            if (active) {
                startContent = Qt.point(input.flickable.contentX, input.flickable.contentY);
            }
        }
        onTranslationChanged: {
            if (active) {
                input.scrollTo(startContent.x - translation.x, startContent.y - translation.y);
            }
        }
    }
}
