#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>
#
# Development only: installs the Peek service menu on the host so Dolphin runs the
# kpeek built in the `dev` distrobox. Undo with: scripts/dev-install-servicemenu.sh --remove

set -e
ROOT=$(cd "$(dirname "$0")/.." && pwd)
DATA="${XDG_DATA_HOME:-$HOME/.local/share}"
MENU="$DATA/kio/servicemenus/kpeek.desktop"
LAUNCH="$HOME/.local/bin/kpeek-launch"
CONFIG="${XDG_CONFIG_HOME:-$HOME/.config}/kpeek-launch.conf"

if [ "$1" = "--remove" ]; then
    rm -f "$MENU" "$LAUNCH" "$CONFIG"
    echo "Removed the development service menu."
    exit 0
fi

mkdir -p "$(dirname "$MENU")" "$(dirname "$LAUNCH")"
install -m 755 "$ROOT/servicemenu/kpeek-launch" "$LAUNCH"
sed "s|@KPEEK_LAUNCH@|$LAUNCH|" "$ROOT/servicemenu/kpeek.desktop.in" > "$MENU"
chmod +x "$MENU" # Dolphin ignores service menus outside system dirs unless they are executable
echo "KPEEK_COMMAND=\"distrobox enter dev -- $ROOT/build/bin/kpeek\"" > "$CONFIG"
echo "Installed $MENU (runs $ROOT/build/bin/kpeek in the dev container)."
