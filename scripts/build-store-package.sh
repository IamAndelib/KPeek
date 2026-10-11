#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>
#
# Builds dist/kpeek-servicemenu-<version>.tar.gz, the package for the KDE Store's "Dolphin Service Menus"
# category: the install/uninstall scripts and the service menu, running the KPeek Flatpak.
set -eu

cd "$(dirname "$0")/.."
version=$(sed -n 's/^project(kpeek VERSION \([0-9.]*\) .*/\1/p' CMakeLists.txt)
stage=$(mktemp -d)
trap 'rm -rf "$stage"' EXIT
pkg="$stage/kpeek-servicemenu"
mkdir -p "$pkg" dist

# The installer installs exactly this release's bundle (built first, by scripts/build-flatpak.sh), checked
# against its checksum.
[ -f kpeek.flatpak ] || { echo "kpeek.flatpak not found: run scripts/build-flatpak.sh first" >&2; exit 1; }
sha256=$(sha256sum kpeek.flatpak | cut -d' ' -f1)
sed -e "s|@BUNDLE_URL@|https://github.com/IamAndelib/KPeek/releases/download/v$version/kpeek.flatpak|" \
    -e "s|@BUNDLE_SHA256@|$sha256|" packaging/kde-store/install-it.sh >"$pkg/install-it.sh"
cp packaging/kde-store/uninstall.sh "$pkg/"
chmod +x "$pkg"/*.sh
sed 's|^Exec=.*|Exec=flatpak run io.github.iamandelib.kpeek %F|' servicemenu/kpeek.desktop >"$pkg/kpeek.desktop"
cp LICENSES/GPL-2.0-or-later.txt "$pkg/LICENSE"

rm -f dist/kpeek-servicemenu-*.tar.gz
tar -czf "dist/kpeek-servicemenu-$version.tar.gz" -C "$stage" kpeek-servicemenu
echo "Wrote $(pwd)/dist/kpeek-servicemenu-$version.tar.gz"
