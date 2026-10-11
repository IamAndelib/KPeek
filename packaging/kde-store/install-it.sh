#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>
#
# Run by Dolphin's "Get New Service Menus" (in a Konsole window) to install KPeek:
# the latest KPeek Flatpak from GitHub, its KDE runtime from Flathub, and the "Peek" service menu.
# Running it again updates KPeek.
set -eu

APP=io.github.iamandelib.kpeek
BUNDLE_URL=https://github.com/IamAndelib/KPeek/releases/latest/download/kpeek.flatpak
MENU_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/kio/servicemenus"
HERE=$(cd "$(dirname "$0")" && pwd)

if ! command -v flatpak >/dev/null 2>&1; then
    echo "KPeek is distributed as a Flatpak, but Flatpak isn't installed."
    echo "Install it with your distribution's package manager (the package is called \"flatpak\"), then try again."
    exit 1
fi

echo "==> Making sure Flathub is available (for the KDE runtime KPeek runs on)"
flatpak remote-add --user --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
echo "==> Downloading the latest KPeek release"
if command -v curl >/dev/null 2>&1; then
    curl -fL --progress-bar -o "$tmp/kpeek.flatpak" "$BUNDLE_URL"
else
    wget -O "$tmp/kpeek.flatpak" "$BUNDLE_URL"
fi

echo "==> Installing KPeek (the first time, this also downloads the KDE runtime, about 1 GB)"
flatpak install --user --reinstall -y "$tmp/kpeek.flatpak"
flatpak info --user "$APP" | grep -E '^ *Version:' || true

echo "==> Adding \"Peek\" to Dolphin's context menu"
mkdir -p "$MENU_DIR"
cp "$HERE/kpeek.desktop" "$MENU_DIR/kpeek.desktop"
chmod +x "$MENU_DIR/kpeek.desktop" # Dolphin ignores user service menus that aren't executable

cat <<EOF

KPeek is installed.

Right-click any file in Dolphin and choose "Peek" (restart Dolphin first if it's open).
To use a keyboard shortcut (Dolphin 26.04 or later):
  1. In Dolphin, open Settings > Configure Keyboard Shortcuts.
  2. Search for "Peek".
  3. Click the shortcut column and press Meta+Space, then Apply.

EOF

# Dolphin runs this in a terminal window that closes as soon as it ends: keep the steps above readable.
if [ -t 0 ]; then
    printf 'Press Enter to close this window.'
    read -r _
fi
