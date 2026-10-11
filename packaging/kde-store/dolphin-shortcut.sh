# shellcheck shell=sh
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>
#
# Sourced by install-it.sh and uninstall.sh: binds Dolphin's "Peek" action (Dolphin 26.04 and later) to
# Meta+Space. Dolphin keeps shortcuts in its KXMLGUI file, one element per line:
#   <ActionProperties scheme="Default">
#     <Action name="servicemenu_kpeek.desktop::peek" shortcut="Meta+Space"/>
# Only sh and awk are certain to be on the host, so this edits lines, not XML.

PEEK_ACTION='servicemenu_kpeek.desktop::peek'
PEEK_SHORTCUT='Meta+Space'
DOLPHIN_RC="${XDG_DATA_HOME:-$HOME/.local/share}/kxmlgui5/dolphin/dolphinui.rc"

# Binds Peek to Meta+Space in $1 (default: Dolphin's file). Returns 0 when it did, 1 when Peek already has a
# shortcut (the user's choice, or an earlier install), 2 when another action has Meta+Space, 3 on an error.
set_shortcut() {
    rc=${1:-$DOLPHIN_RC}
    line="  <Action name=\"$PEEK_ACTION\" shortcut=\"$PEEK_SHORTCUT\"/>"
    if [ ! -e "$rc" ]; then
        # Dolphin's built-in file has a higher version: KXMLGUI merges these properties into it on start.
        mkdir -p "$(dirname "$rc")" || return 3
        printf '<!DOCTYPE gui>\n<gui name="dolphin" version="1">\n <ActionProperties scheme="Default">\n%s\n </ActionProperties>\n</gui>\n' \
            "$line" >"$rc.tmp" && mv "$rc.tmp" "$rc" || return 3
        return 0
    fi
    grep -q "name=\"$PEEK_ACTION\"" "$rc" && return 1
    grep -q "shortcut=\"\([^\"]*; *\)\{0,1\}${PEEK_SHORTCUT}[;\"]" "$rc" && return 2
    awk -v line="$line" '
        !done && /<ActionProperties scheme="Default" *\/>/ {
            sub(/ *\/>/, ">"); print; print line; print " </ActionProperties>"; done = 1; next
        }
        /<ActionProperties scheme="Default"/ { inblock = 1 }
        inblock && !done && /<\/ActionProperties>/ { print line; done = 1 }
        !done && /<\/gui>/ { print " <ActionProperties scheme=\"Default\">"; print line; print " </ActionProperties>"; done = 1 }
        { print }
        END { exit !done }
    ' "$rc" >"$rc.tmp" || { rm -f "$rc.tmp"; return 3; }
    cp "$rc" "$rc.kpeek-backup" && mv "$rc.tmp" "$rc" || return 3
}

# Removes Peek's shortcut from $1 (default: Dolphin's file).
remove_shortcut() {
    rc=${1:-$DOLPHIN_RC}
    grep -q "name=\"$PEEK_ACTION\"" "$rc" 2>/dev/null || return 0
    grep -v "name=\"$PEEK_ACTION\"" "$rc" >"$rc.tmp"
    mv "$rc.tmp" "$rc"
}
