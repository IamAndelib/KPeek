#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>
#
# Development only: adds the "Peek" service menu to the host's Dolphin, running the kpeek
# built in the `dev` distrobox. Undo with: scripts/dev-install-servicemenu.sh --remove

set -eu

ROOT=$(cd "$(dirname "$0")/.." && pwd)
MENU="${XDG_DATA_HOME:-$HOME/.local/share}/kio/servicemenus/kpeek.desktop"

if [ "${1:-}" = "--remove" ]; then
    rm -f "$MENU"
    echo "Removed $MENU"
    exit 0
fi

case $ROOT in
*[\"\`\$\\]*)
    echo "The checkout path contains characters a .desktop Exec line can't quote: $ROOT" >&2
    exit 1
    ;;
esac

mkdir -p "$(dirname "$MENU")"
while IFS= read -r line; do
    case $line in
    Exec=*) printf 'Exec=distrobox enter dev -- "%s/build/bin/kpeek" %%F\n' "$ROOT" ;;
    *) printf '%s\n' "$line" ;;
    esac
done <"$ROOT/servicemenu/kpeek.desktop" >"$MENU"
chmod +x "$MENU" # Dolphin ignores user service menus that aren't executable

echo "Installed $MENU (runs $ROOT/build/bin/kpeek in the dev container). Restart Dolphin to pick it up."
