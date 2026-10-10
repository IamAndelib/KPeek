# KPeek

Quick Look / PowerToys-Peek style previewer for Dolphin (Qt 6, KF6, Kirigami/QML, C++20, CMake/ECM).
Shipped as a Flatpak (Flathub) plus a KDE Store "Dolphin Service Menu" whose `install.sh` installs it.
The design and milestones are in `~/.claude/plans/we-need-to-create-zippy-star.md`.

## Dev environment: never install dev packages on the host
All C++ builds and tests run inside the shared `dev` distrobox (Fedora 44, matching the host Qt/KF6; defined in `~/.config/distrobox/dev.ini`, where new packages go):

    distrobox enter dev -- cmake -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    distrobox enter dev -- cmake --build build
    distrobox enter dev -- ctest --test-dir build --output-on-failure
    distrobox enter dev -- ./build/bin/kpeek <file>...   # GUI runs on the host session (Wayland + D-Bus shared)

KPeek stays resident after a preview, so **kill the old instance after rebuilding** or new runs just forward to it:
`pkill -f build/bin/kpeek`. Qt logs go to the journal unless you set `QT_FORCE_STDERR_LOGGING=1`.

Flatpak builds run on the host through the sandboxed builder (user-level flatpaks, no host packages):

    flatpak run org.flatpak.Builder --user --install --force-clean build-flatpak packaging/flatpak/<appid>.yml

Commit from inside the container too (`distrobox enter dev -- git commit …`): ECM's pre-commit hook needs `git-clang-format`,
which is only installed there. Format with `distrobox enter dev -- cmake --build build --target clang-format`.

## Layout
- `src/`: C++ backend (`PeekController` handles navigation, toggling and D-Bus) and `src/qml/` (the UI, one file per viewer under `viewers/`)
- `servicemenu/`: `kpeek.desktop` service menu, `kpeek-launch` host wrapper, KDE Store `install.sh`/`uninstall.sh`
- `packaging/flatpak/`: Flathub manifest
- `autotests/`: QtTest

## Conventions
- KDE coding style (`clang-format` via ECM's KDEClangFormat); pick viewers by MIME type through QMimeDatabase, never by file extension.
- Viewers must work inside the Flatpak KDE runtime, so they can't depend on host KParts or host binaries.
- Tests that touch Dolphin's `dolphinui.rc` work on fixture copies only.
