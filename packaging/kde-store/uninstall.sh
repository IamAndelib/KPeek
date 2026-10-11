#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>
#
# Run by Dolphin's "Get New Service Menus" when KPeek is uninstalled there.
set -u

APP=io.github.iamandelib.kpeek
MENU="${XDG_DATA_HOME:-$HOME/.local/share}/kio/servicemenus/kpeek.desktop"

rm -f "$MENU"
echo "Removed the \"Peek\" service menu."

if flatpak info --user "$APP" >/dev/null 2>&1; then
    flatpak uninstall --user -y "$APP"
    echo "Removed KPeek. (The KDE runtime stays; \"flatpak uninstall --unused\" removes it if nothing else needs it.)"
fi

if [ -t 0 ]; then
    printf 'Press Enter to close this window.'
    read -r _
fi
