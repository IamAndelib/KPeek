# KPeek

Quick Look / PowerToys Peek–style previewer for KDE Dolphin: Qt 6, KF6, Kirigami/QML, C++20, CMake/ECM.
Read README.md first; it covers the features, the Dolphin integration (service menu + shortcut) and building.

## Principles
- KISS: lean code, the simplest change that works; no speculative abstractions or options.
- Viewers are picked by MIME type (`src/viewerresolver.cpp`), never by file extension.
- Viewers must work inside the Flatpak KDE runtime: no host KParts or host binaries. Native builds convert
  office documents with LibreOffice; the Flatpak never reaches the host (that permission would let any
  decoder bug escape the sandbox) and shows the document's embedded preview image instead. It reads the
  host's `/usr` and `/etc` under `/run/host` (`main.cpp` maps them).
- Files are untrusted: render file-derived strings (names, tags, entry paths) with `Text.PlainText`; the
  QML engine refuses network access (`LocalOnlyNetworkAccessManager` in `main.cpp`); never run external
  programs on file content (libarchive filters are listed explicitly in `src/archivereader.h`, EPS is excluded and
  `src/execguard.cpp` refuses to execute Ghostscript at all: Qt detects image formats from content, past any MIME check).
  Anything a file *references* is untrusted too: Markdown goes through `TextLoader::markdownToHtml()` (Qt's text
  engine reads every image a document names, whole, on the GUI thread: `![](/dev/zero)` never ends), glTF counts
  its referenced files before auto-loading (`PeekController::modelBytes()`), and native LibreOffice runs
  without network access (`unshare --map-root-user --net`, or `flatpak run --unshare=network`).
- Shared interaction lives in `src/qml/ViewerInput.qml` (Ctrl+wheel zoom, Ctrl+drag, scroll keys) and the
  header zoom controls in `Main.qml`. Viewers take part by exposing `zoom` and `scrollTarget` (optionally `dragPans`, `maxZoom`); don't add
  per-viewer controls for these. The plain wheel only scrolls (never switches files: documents need it).
- Images follow GNOME Sushi / Gwenview: decode once at screen size, GPU-scale while zooming, and load full
  detail (≤ 8192 px, ≤ 256 MP) once, on top, only when zooming needs it. Never hide pixels before their
  replacement is ready (that caused zoom flashing), and never set an Image.sourceSize larger than the image:
  Qt scales *up* to it. Dolphin's thumbnail cache (`src/thumbnailcache.cpp`) is the instant placeholder;
  video decodes on the GPU via VA-API.
- Zoom must update content size and scroll position in the same handler, from the zoom formula: never
  `Qt.callLater` it, and never set a ListView's contentX before its content has grown (it gets clamped).
  A deferred scroll position renders one frame at the old position, which flickers above 100%.
- Machines with ≤ 4 GB RAM (`isLowMemorySystem()`, `src/systeminfo.h`; `KPEEK_LOW_MEMORY=1` forces it)
  get lower caps: images 4096 px, PDF pages 24 MP, 2 cached PDF documents.
- Flatpak manifest (when written) needs: `--device=dri` (GPU and VA-API), the `codecs-extra` extension
  (H.264/HEVC), `--filesystem=host:ro` (sibling files), `--filesystem=xdg-cache/thumbnails:ro`.
  No `--share=network` and no host access (`org.freedesktop.Flatpak`). Don't bundle NVIDIA libraries.
- Paged documents (PDF, office, comics) share `src/qml/PageView.qml`. A new paged format is a C++ QML type with
  `pageCount`, `pageSize(i)`, `pageSource(i)` (an async image provider URL) and `failed`, like `ComicBook`.
- Archives and comic books are read with libarchive (streamed headers, no in-memory tree); its one-thread
  `archivePool()` keeps a slow listing from starving image decodes.
- 3D models use Qt Quick 3D, which can't run on the software renderer: `ModelViewer.qml` never creates its scene
  there. Headless tests of it need real GL: run under `Xvfb` with `QT_QPA_PLATFORM=xcb`, not `offscreen`.
- Formats with no viewer (camera RAW, Blender…) fall back to Dolphin's thumbnail on the info card; don't add
  decoders the Flatpak runtime lacks.
- A second invocation forwards its arguments in `src/forwarder.cpp` (plain libdbus, before QApplication
  exists, saving 40–70 ms per key press). It speaks KDBusService's `CommandLine` protocol: keep the two in sync.
- No resident process: closing the preview quits KPeek (a windowless Flatpak would show up in Plasma's
  system tray as a background app). `main()` limits glibc to two malloc arenas (decoder threads otherwise
  strand hundreds of MB during a session). Large 3D models load only on request. Measure before/after with
  `/proc/<pid>/smaps_rollup` (Private_Dirty), not RSS.
- The window is frameless: the header bar moves it (a `DragHandler` calling `startSystemMove()`; buttons keep their
  clicks), edge strips on the window's root item (`root.header.parent`, stacked above the header; anywhere lower the header
  swallows the pointer at the top edge) resize it, and `KWindowShadow` asks the compositor for a shadow.
- Background work goes on `workerPool()` (`src/workerpool.h`), libarchive scans on `archivePool()`; never
  `QThreadPool::globalInstance()`.
- Never set a KSyntaxHighlighting `SyntaxHighlighter.textEdit` to null: it crashes. Swap components instead.
- Qt on Wayland reports wheel events (even from a real mouse) with a "touchpad"-typed device: any
  `WheelHandler` needs `acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad`. QtQuickTest's
  `mouseWheel()` only simulates mouse devices, so test this in C++ (see `autotests/viewerinputtest.cpp`).

## Workflow
- Build in a container with the dependencies from `.github/workflows/ci.yml` (the maintainer uses a Fedora
  distrobox named `dev`: prefix commands with `distrobox enter dev --`).
- Build and test: `cmake -B build -G Ninja && cmake --build build && ctest --test-dir build`.
- An open preview receives new runs (they forward to it): close it, or `pkill -x kpeek`, after rebuilding.
- Qt logs go to the journal unless `QT_FORCE_STDERR_LOGGING=1` is set.
- For headless runs, use a private bus and no portal:
  `dbus-run-session -- env QT_QPA_PLATFORM=offscreen QT_NO_XDG_DESKTOP_PORTAL=1 ./build/bin/kpeek <file>`.
- Before committing: `cmake --build build --target clang-format`, `reuse lint`. The pre-commit hook needs `git-clang-format`.
