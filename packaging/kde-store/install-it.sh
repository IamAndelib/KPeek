#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>
#
# Run by Dolphin's "Get New Service Menus" (in a Konsole window) to install KPeek:
# this release's KPeek Flatpak from GitHub, its KDE runtime from Flathub, and the "Peek" service menu.
# scripts/build-store-package.sh fills in the release's bundle and its checksum.
set -eu

tmp=
finish() {
    [ -z "$tmp" ] || rm -rf "$tmp"
    # Dolphin runs this in a terminal window that closes as soon as it ends: keep the output readable,
    # including an error.
    if [ -t 0 ]; then
        printf 'Press Enter to close this window.'
        read -r _
    fi
}
trap finish EXIT

APP=io.github.iamandelib.kpeek
BUNDLE_URL=@BUNDLE_URL@
BUNDLE_SHA256=@BUNDLE_SHA256@
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
echo "==> Downloading KPeek"
if command -v curl >/dev/null 2>&1; then
    curl -fL --proto '=https' --progress-bar -o "$tmp/kpeek.flatpak" "$BUNDLE_URL"
else
    wget --https-only -O "$tmp/kpeek.flatpak" "$BUNDLE_URL"
fi
echo "$BUNDLE_SHA256  $tmp/kpeek.flatpak" | sha256sum -c --quiet - || {
    echo "The download doesn't match this release's checksum: not installing it."
    exit 1
}

echo "==> Installing KPeek (the first time, this also downloads the KDE runtime, about 400 MB, unless another app already installed it)"
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
