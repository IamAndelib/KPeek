#!/bin/sh
# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>
#
# Builds the KPeek Flatpak from the working tree and exports a single-file bundle.
#   scripts/build-flatpak.sh            # build, install for this user, write kpeek.flatpak
#   scripts/build-flatpak.sh --no-install
# Needs flatpak, flatpak-builder (or org.flatpak.Builder) and the KDE SDK:
#   flatpak install --user flathub org.kde.Sdk//6.11 org.kde.Platform//6.11
set -eu

cd "$(dirname "$0")/.."
manifest=packaging/flatpak/io.github.iamandelib.kpeek.yml
install=--install
[ "${1:-}" = --no-install ] && install=

if ! flatpak info org.kde.Sdk//6.11 >/dev/null 2>&1; then
    echo "The KDE SDK is missing: flatpak install --user flathub org.flatpak.Builder org.kde.Sdk//6.11 org.kde.Platform//6.11" >&2
    exit 1
fi

# A native flatpak-builder if there is one: the Flatpak'd builder only finds SDKs installed system-wide.
builder="flatpak run --command=flatpak-builder org.flatpak.Builder"
command -v flatpak-builder >/dev/null 2>&1 && builder=flatpak-builder

# --disable-rofiles-fuse: containers (e.g. distrobox) don't allow FUSE mounts; it only costs some speed.
$builder --user $install --force-clean --disable-rofiles-fuse \
    --repo=flatpak-repo flatpak-build "$manifest"

# The bundle installs from a file; the KDE runtime still comes from Flathub.
flatpak build-bundle --runtime-repo=https://dl.flathub.org/repo/flathub.flatpakrepo \
    flatpak-repo kpeek.flatpak io.github.iamandelib.kpeek
echo "Wrote $(pwd)/kpeek.flatpak"
