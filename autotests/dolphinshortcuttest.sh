#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>
#
# Tests packaging/kde-store/dolphin-shortcut.sh, which binds Peek to Meta+Space in Dolphin's dolphinui.rc.
set -u

# shellcheck source=packaging/kde-store/dolphin-shortcut.sh
. "$(dirname "$0")/../packaging/kde-store/dolphin-shortcut.sh"
dir=$(mktemp -d)
trap 'rm -rf "$dir"' EXIT
failed=0
ours='  <Action name="servicemenu_kpeek.desktop::peek" shortcut="Meta+Space"/>'
same() { [ "$(cksum <"$1")" = "$(cksum <"$2")" ]; } # byte-identical (cmp is not in every container)

check() { # name, expected status, file, expected content (empty: unchecked)
    status=0
    set_shortcut "$3" || status=$?
    if [ "$status" != "$2" ]; then
        echo "FAIL $1: status $status, expected $2"; failed=1; return
    fi
    if [ -n "$4" ] && [ "$(cat "$3")" != "$4" ]; then
        echo "FAIL $1: got"; cat "$3"; failed=1; return
    fi
    if [ "$2" = 0 ] && command -v xmllint >/dev/null && ! xmllint --noout "$3"; then
        echo "FAIL $1: not well-formed"; failed=1; return
    fi
    echo "PASS $1"
}

check "no file" 0 "$dir/new/dolphinui.rc" "<!DOCTYPE gui>
<gui name=\"dolphin\" version=\"1\">
 <ActionProperties scheme=\"Default\">
$ours
 </ActionProperties>
</gui>"
check "no file, again" 1 "$dir/new/dolphinui.rc" ""

printf '<!DOCTYPE gui>\n<gui name="dolphin" version="49">\n <ActionProperties scheme="Default">\n  <Action name="go_back" priority="0"/>\n </ActionProperties>\n</gui>\n' >"$dir/block.rc"
cp "$dir/block.rc" "$dir/block.orig"
check "existing block" 0 "$dir/block.rc" "<!DOCTYPE gui>
<gui name=\"dolphin\" version=\"49\">
 <ActionProperties scheme=\"Default\">
  <Action name=\"go_back\" priority=\"0\"/>
$ours
 </ActionProperties>
</gui>"
same "$dir/block.rc.kpeek-backup" "$dir/block.orig" || { echo "FAIL backup"; failed=1; }
check "existing block, again" 1 "$dir/block.rc" ""
remove_shortcut "$dir/block.rc"
if same "$dir/block.rc" "$dir/block.orig"; then echo "PASS remove"; else echo "FAIL remove"; failed=1; fi

printf '<!DOCTYPE gui>\n<gui name="dolphin" version="49">\n <ActionProperties scheme="Default"/>\n</gui>\n' >"$dir/closed.rc"
check "self-closed block" 0 "$dir/closed.rc" "<!DOCTYPE gui>
<gui name=\"dolphin\" version=\"49\">
 <ActionProperties scheme=\"Default\">
$ours
 </ActionProperties>
</gui>"

printf '<!DOCTYPE gui>\n<gui name="dolphin" version="49">\n <MenuBar/>\n</gui>\n' >"$dir/none.rc"
check "no block" 0 "$dir/none.rc" "<!DOCTYPE gui>
<gui name=\"dolphin\" version=\"49\">
 <MenuBar/>
 <ActionProperties scheme=\"Default\">
$ours
 </ActionProperties>
</gui>"

printf '<gui name="dolphin" version="49">\n <ActionProperties scheme="Default">\n  <Action name="servicemenu_kpeek.desktop::peek" shortcut="Space"/>\n </ActionProperties>\n</gui>\n' >"$dir/user.rc"
cp "$dir/user.rc" "$dir/user.orig"
check "user's own shortcut" 1 "$dir/user.rc" "$(cat "$dir/user.orig")"

printf '<gui name="dolphin" version="49">\n <ActionProperties scheme="Default">\n  <Action name="foo" shortcut="Ctrl+X; Meta+Space"/>\n </ActionProperties>\n</gui>\n' >"$dir/taken.rc"
check "key taken" 2 "$dir/taken.rc" "$(cat "$dir/taken.rc")"

printf '<gui name="dolphin" version="49">\n <ActionProperties scheme="Default">\n  <Action name="foo" shortcut="Ctrl+Meta+Space"/>\n </ActionProperties>\n</gui>\n' >"$dir/similar.rc"
check "similar key is free" 0 "$dir/similar.rc" ""

printf '<gui name="dolphin" version="49">\n' >"$dir/broken.rc"
check "truncated file" 3 "$dir/broken.rc" '<gui name="dolphin" version="49">'

exit $failed
