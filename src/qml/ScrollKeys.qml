// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibsriz@proton.me>

pragma Singleton

import QtQuick
import org.kde.kirigami as Kirigami

// Keyboard scrolling for viewers. Left/Right stay reserved for moving between files.
QtObject {
    function handle(event, flickable) {
        let target = flickable.contentY;
        const lineStep = Kirigami.Units.gridUnit * 3;
        const pageStep = flickable.height * 0.9;
        switch (event.key) {
        case Qt.Key_Up: target -= lineStep; break;
        case Qt.Key_Down: target += lineStep; break;
        case Qt.Key_PageUp: target -= pageStep; break;
        case Qt.Key_PageDown: target += pageStep; break;
        case Qt.Key_Home: target = flickable.originY; break;
        case Qt.Key_End: target = flickable.originY + flickable.contentHeight; break;
        default: return;
        }
        const maxY = flickable.originY + Math.max(0, flickable.contentHeight - flickable.height);
        flickable.contentY = Math.max(flickable.originY, Math.min(target, maxY));
        event.accepted = true;
    }
}
