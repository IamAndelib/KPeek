#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>
#
# Builds dist/kpeek-servicemenu.tar.gz, the package for the KDE Store's "Dolphin Service Menus"
# category: the install/uninstall scripts and the service menu, running the KPeek Flatpak.
set -eu

cd "$(dirname "$0")/.."
stage=$(mktemp -d)
trap 'rm -rf "$stage"' EXIT
pkg="$stage/kpeek-servicemenu"
mkdir -p "$pkg" dist

cp packaging/kde-store/install-it.sh packaging/kde-store/uninstall.sh "$pkg/"
chmod +x "$pkg"/*.sh
sed 's|^Exec=.*|Exec=flatpak run io.github.iamandelib.kpeek %F|' servicemenu/kpeek.desktop >"$pkg/kpeek.desktop"
cp LICENSES/GPL-2.0-or-later.txt "$pkg/LICENSE"

tar -czf dist/kpeek-servicemenu.tar.gz -C "$stage" kpeek-servicemenu
echo "Wrote $(pwd)/dist/kpeek-servicemenu.tar.gz"
