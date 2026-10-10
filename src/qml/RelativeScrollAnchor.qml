// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

import QtQuick

/**
 * Keeps a Flickable at the same relative place in its content when the content height changes,
 * as it does when zooming scales text or the window is resized. The new position is set in the
 * same handler as the change, so no frame shows the content jumped (see CLAUDE.md on zooming).
 * At the top it stays at the top.
 */
Item {
    id: anchor

    required property Flickable flickable
    property real fraction: 0
    property bool restoring: false

    Connections {
        target: anchor.flickable
        function onContentYChanged() {
            const f = anchor.flickable;
            if (!anchor.restoring && f.contentHeight > 0) {
                anchor.fraction = f.contentY <= f.originY ? 0 : (f.contentY - f.originY + f.height / 2) / f.contentHeight;
            }
        }
        function onContentHeightChanged() {
            const f = anchor.flickable;
            if (anchor.fraction === 0) {
                return;
            }
            anchor.restoring = true;
            f.contentY = f.originY + Math.max(0, Math.min(anchor.fraction * f.contentHeight - f.height / 2, f.contentHeight - f.height));
            anchor.restoring = false;
        }
    }
}
