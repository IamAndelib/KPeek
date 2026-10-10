# KPeek

Quick Look / PowerToys Peek–style previewer for KDE Dolphin: Qt 6, KF6, Kirigami/QML, C++20, CMake/ECM.
Read README.md first; it covers the features, the Dolphin integration (service menu + shortcut) and building.

## Principles
- KISS: lean code, the simplest change that works; no speculative abstractions or options.
- Viewers are picked by MIME type (`src/viewerresolver.cpp`), never by file extension.
- Viewers must work inside the Flatpak KDE runtime: no host KParts or host binaries (LibreOffice is the
  one optional exception, reached through `flatpak-spawn --host`).
- Shared interaction lives in `src/qml/ViewerInput.qml` (Ctrl+wheel zoom, Ctrl+drag, scroll keys) and the
  header zoom controls in `Main.qml`. Viewers take part by exposing `zoom` and `scrollTarget`; don't add
  per-viewer controls for these. The plain wheel only scrolls (never switches files: documents need it).
- Images follow GNOME Sushi / Gwenview: decode once at screen size, GPU-scale while zooming, and load full
  detail (≤ 8192 px, ≤ 256 MP) once, on top, only when zooming needs it. Never hide pixels before their
  replacement is ready (that caused zoom flashing), and never set an Image.sourceSize larger than the image:
  Qt scales *up* to it. Dolphin's thumbnail cache (`src/thumbnailcache.cpp`) is the instant placeholder;
  video decodes on the GPU via VA-API. The Flatpak manifest must grant `--filesystem=xdg-cache/thumbnails:ro`.
- Zoom must update content size and scroll position in the same handler, from the zoom formula: never
  `Qt.callLater` it, and never set a ListView's contentX before its content has grown (it gets clamped).
  A deferred scroll position renders one frame at the old position, which flickers above 100%.
- Background work goes on `workerPool()` (`src/workerpool.h`), never `QThreadPool::globalInstance()`.
- Never set a KSyntaxHighlighting `SyntaxHighlighter.textEdit` to null: it crashes. Swap components instead.
- Qt on Wayland reports wheel events (even from a real mouse) with a "touchpad"-typed device: any
  `WheelHandler` needs `acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad`. QtQuickTest's
  `mouseWheel()` only simulates mouse devices, so test this in C++ (see `autotests/viewerinputtest.cpp`).

## Workflow
- Build in a container with the dependencies from `.github/workflows/ci.yml` (the maintainer uses a Fedora
  distrobox named `dev`: prefix commands with `distrobox enter dev --`).
- Build and test: `cmake -B build -G Ninja && cmake --build build && ctest --test-dir build`.
- KPeek stays resident after a preview: `pkill -x kpeek` after rebuilding, or new runs forward to the old process.
- Qt logs go to the journal unless `QT_FORCE_STDERR_LOGGING=1` is set.
- For headless runs, use a private bus and no portal:
  `dbus-run-session -- env QT_QPA_PLATFORM=offscreen QT_NO_XDG_DESKTOP_PORTAL=1 ./build/bin/kpeek <file>`.
- Before committing: `cmake --build build --target clang-format`, `reuse lint`. The pre-commit hook needs `git-clang-format`.
