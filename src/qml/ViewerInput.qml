// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

import QtQuick
import org.kde.kirigami as Kirigami

/**
 * Wheel, drag and key handling shared by all viewers. It sits on top of the viewer.
 *
 * A viewer takes part by exposing `scrollTarget` (its Flickable, or null when it has
 * nothing to scroll), a `zoom` property if zooming means something for it, and
 * `dragPans: true` if a plain drag pans its content (nothing to select, as in images), and
 * `maxZoom` if it should zoom further than 400%.
 *  - Ctrl+wheel zooms. The plain wheel isn't touched: it scrolls the viewer natively.
 *  - Ctrl+left-drag pans; so does a plain drag in `dragPans` viewers (their Flickable does it).
 *    A hand cursor hints at both: open while hovering, closed while the button is held.
 *  - Up/Down/PgUp/PgDn/Home/End scroll.
 *
 * Debug output: QT_LOGGING_RULES="kpeek.input.debug=true"
 */
Item {
    id: input

    property Item viewer
    property real zoom: 1.0
    readonly property Flickable flickable: viewer?.scrollTarget ?? null

    readonly property bool zoomable: viewer !== null && "zoom" in viewer
    readonly property bool dragPans: viewer?.dragPans ?? false
    readonly property bool pannable: flickable !== null && (flickable.contentHeight > flickable.height + 1 || flickable.contentWidth > flickable.width + 1)
    readonly property real minZoom: 0.25
    // A viewer may allow more, e.g. images, so large photos can be seen past their real pixels.
    readonly property real maxZoom: viewer?.maxZoom ?? 4.0

    function zoomBy(steps) {
        zoom = Math.min(maxZoom, Math.max(minZoom, zoom * Math.pow(1.25, steps)));
    }

    function scrollTo(x, y) {
        const f = flickable;
        f.contentX = Math.max(f.originX, Math.min(x, f.originX + Math.max(0, f.contentWidth - f.width)));
        f.contentY = Math.max(f.originY, Math.min(y, f.originY + Math.max(0, f.contentHeight - f.height)));
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

    LoggingCategory {
        id: log
        name: "kpeek.input"
        defaultLogLevel: LoggingCategory.Warning
    }

    WheelHandler {
        target: null
        // WheelHandler only takes mouse-type devices by default, but Qt on Wayland reports wheel
        // events from a "touchpad"-typed device, even for a real mouse wheel.
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        acceptedModifiers: Qt.ControlModifier
        enabled: input.zoomable
        // 120 per mouse wheel notch; touchpads send smaller steps for smooth zooming.
        onWheel: event => {
            console.debug(log, "Ctrl+wheel", event.angleDelta, event.pixelDelta, "zoom", input.zoom);
            input.zoomBy(event.angleDelta.y / 120);
        }
    }

    // Grab-hand hints. Neither handler grabs the pointer, so the viewer's own dragging is unaffected.
    HoverHandler {
        enabled: input.pannable && input.dragPans
        cursorShape: Qt.OpenHandCursor
    }
    PointHandler {
        enabled: input.pannable
        acceptedButtons: Qt.LeftButton
        acceptedModifiers: input.dragPans ? Qt.KeyboardModifierMask : Qt.ControlModifier
        cursorShape: Qt.ClosedHandCursor
    }

    DragHandler {

        property point startContent

        target: null
        acceptedModifiers: Qt.ControlModifier
        // Viewers where a plain drag pans do it with their own Flickable; this adds Ctrl+drag everywhere else.
        enabled: input.pannable && !input.dragPans
        cursorShape: Qt.ClosedHandCursor
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
