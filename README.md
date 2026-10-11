# KPeek

**Quick Look for Dolphin.** Select a file in KDE's file manager, press <kbd>Meta</kbd>+<kbd>Space</kbd>, and a preview pops up.
Use the arrow keys to move to the next file, and press the shortcut again to close the preview.

KPeek is to Dolphin what [PowerToys Peek](https://learn.microsoft.com/windows/powertoys/peek) is to Windows Explorer
and [Sushi](https://gitlab.gnome.org/GNOME/sushi) is to GNOME Files.

![KPeek previewing an image](docs/screenshots/image.png)

## Install

KPeek runs as a Flatpak, so it works on any distribution with KDE Plasma 6 and Flatpak.

**From Dolphin (recommended).** Open *Settings → Configure Dolphin… → Context Menu → Download New Services…*,
search for **KPeek** and click *Install*. A terminal window shows the progress: it downloads KPeek, and the
first time also the KDE runtime it runs on (about 1 GB, shared with other KDE Flatpaks). Restart Dolphin
afterwards. Installing it again later updates KPeek.

**By hand.** Download `kpeek.flatpak` and `kpeek-servicemenu.tar.gz` from the
[latest release](https://github.com/IamAndelib/KPeek/releases/latest), then:

```sh
flatpak install --user kpeek.flatpak
tar -xzf kpeek-servicemenu.tar.gz
kpeek-servicemenu/install-it.sh   # also downloads the newest release and adds "Peek" to Dolphin
```

**Set the shortcut** (Dolphin 26.04 or later): in Dolphin, open *Settings → Configure Keyboard Shortcuts*,
search for *Peek*, and assign <kbd>Meta</kbd>+<kbd>Space</kbd>. Without a shortcut, right-click a file and choose *Peek*.

**Uninstall** from the same *Download New Services* dialog, or run `kpeek-servicemenu/uninstall.sh`.

## What it previews

| Type | Preview |
|---|---|
| Images (JPEG, PNG, WebP, AVIF, JXL, HEIC, SVG…) and GIF/WebP animations | Fitted to the window |
| PDF | Every page, with scrolling, a page counter and zoom |
| Office documents (docx, odt, xlsx, ods, pptx, odp, doc, rtf…) | The Flatpak shows the first-page picture stored in the document (most ODF files and many Office files have one); it never reaches outside its sandbox. Builds from source render every page through LibreOffice |
| Video and audio | Starts playing immediately; seek bar, mute, and title/artist for audio |
| Source code and text | Syntax highlighting that follows your color scheme |
| Markdown | Rendered, with a source toggle |
| Comic books (cbz, cbr, cb7, cbt) | Every page, like a PDF |
| 3D models (glTF/GLB, OBJ, STL, 3MF) | Drag to orbit; needs GPU rendering |
| Archives (zip, tar.*, 7z, rar, iso, deb, rpm, cab, cpio…) | Contents tree with sizes |
| Fonts | Specimen at several sizes |
| Folders and everything else (camera RAW, Blender, …) | Dolphin's thumbnail when it has one, else the icon; type, size and modification date |

### Light on resources

- **Images** are decoded once at screen size, however many megapixels they have. The first zoom past that
  loads full detail once (up to 8192 px a side); zooming itself is done by the graphics card, so it is smooth
  and never flashes. A 24-megapixel photo peaks around 225 MB, a 384-megapixel one stays near 125 MB.
- **Dolphin's thumbnails** are shown instantly as placeholders while the full image or video loads.
- **Video** is decoded by the graphics card where possible (VA-API). A large video in a codec the card
  probably can't decode (e.g. 4K AV1) waits for you to press Play instead of loading the processor.
- **Machines with 4 GB of memory or less** get lower limits for image and PDF detail automatically.
- **When hidden**, KPeek stays in the background for 10 minutes so the next preview opens instantly,
  but hands back the memory used by the last preview.

### Troubleshooting

- Video plays badly or not at all with a broken GPU driver: start with `QT_FFMPEG_DECODING_HW_DEVICE_TYPES=,`
  to decode on the processor.
- Rendering problems with an unusual GPU: try `QSG_RHI_BACKEND=vulkan`, or `QT_QUICK_BACKEND=software`.
- Force the low-memory limits: `KPEEK_LOW_MEMORY=1`.

### Keys and mouse

With several files selected, KPeek moves through those; with one, through its whole folder.

| Input | Action |
|---|---|
| <kbd>←</kbd> <kbd>→</kbd> | Previous / next file |
| Mouse wheel, <kbd>↑</kbd> <kbd>↓</kbd> <kbd>PgUp</kbd> <kbd>PgDn</kbd> <kbd>Home</kbd> <kbd>End</kbd> | Scroll |
| <kbd>Ctrl</kbd>+<kbd>+</kbd> <kbd>Ctrl</kbd>+<kbd>−</kbd> <kbd>Ctrl</kbd>+<kbd>0</kbd>, <kbd>Ctrl</kbd>+wheel | Zoom in, out, reset (everything but media and archives). The header shows the zoom level, with zoom buttons and a reset button. |
| <kbd>Ctrl</kbd>+drag | Grab and move the content; zoomed images and document pages also pan with a plain drag |
| <kbd>Enter</kbd> | Open in the default application |
| <kbd>Meta</kbd>+<kbd>Space</kbd>, <kbd>Space</kbd>, <kbd>Esc</kbd>, <kbd>Q</kbd> | Close |
| <kbd>K</kbd>, <kbd>J</kbd> / <kbd>L</kbd>, <kbd>M</kbd> | Play/pause, back/forward 10 seconds, mute (media) |
| <kbd>Ctrl</kbd>+<kbd>U</kbd> | Show the Markdown source |

## How it integrates with Dolphin

Dolphin has no plugin API for reading the current selection. KPeek therefore ships as a **service menu**: a "Peek" entry in
Dolphin's context menu that receives the selected files. Dolphin 26.04 and later let you put a keyboard shortcut on a service menu
action, so you can bind it under *Settings → Configure Keyboard Shortcuts → Peek*. The suggested shortcut is <kbd>Meta</kbd>+<kbd>Space</kbd>;
plain Space already toggles Dolphin's selection mode.

KPeek runs as a single instance. After the first preview it stays in the background for 10 minutes, so the next one opens instantly.
Invoking it again on the file that's already showing closes the preview.

## Requirements

- KDE Plasma 6 with Dolphin ≥ 26.04 (needed for the keyboard shortcut; the context menu entry works with older versions)
- To build from source: Qt ≥ 6.8, KDE Frameworks ≥ 6.10, Kirigami, Poppler (Qt 6), libarchive, libdbus, QtMultimedia,
  Qt Quick 3D, KSyntaxHighlighting. Optional: LibreOffice, for rendering every page of office documents

## Building

```sh
cmake -B build -G Ninja
cmake --build build
ctest --test-dir build
./build/bin/kpeek some-file.pdf
```

### Development setup

Development happens in a Fedora [distrobox](https://distrobox.it/) so the host system stays clean. To try a build from
your real Dolphin:

```sh
scripts/dev-install-servicemenu.sh          # adds "Peek" to Dolphin, running ./build/bin/kpeek
scripts/dev-install-servicemenu.sh --remove # undo
```

Then bind the shortcut as described above and restart Dolphin.

To build the Flatpak bundle and the KDE Store package (needs `flatpak-builder` and the KDE 6.11 SDK):

```sh
scripts/build-flatpak.sh        # writes kpeek.flatpak and installs it for your user
scripts/build-store-package.sh  # writes dist/kpeek-servicemenu.tar.gz
```

Pushing a `v*` tag builds both and attaches them to a GitHub release. After rebuilding, run `pkill -x kpeek`,
because the resident instance keeps running the old binary. Set `QT_FORCE_STDERR_LOGGING=1` to see Qt's
messages in the terminal instead of the system journal.

## Contributing

Bug reports and pull requests are welcome on [GitHub](https://github.com/IamAndelib/KPeek).

- **Style:** KDE's [coding style](https://community.kde.org/Policies/Frameworks_Coding_Style), enforced by clang-format.
  Run `cmake --build build --target clang-format` before committing; the pre-commit hook that CMake installs checks it.
- **Tests:** add or update a test in `autotests/` for logic changes (C++ with QtTest, UI behaviour as
  `autotests/qml/tst_*.qml`), and keep `ctest --test-dir build` green.
- **Layout:** `src/` holds the C++ backend, with `src/qml/viewers/` holding one QML file per preview type. A new
  preview type is a `XxxViewer.qml` plus a MIME rule in `src/viewerresolver.cpp`. Viewers take part in
  zoom, drag and keyboard scrolling by exposing `zoom`, `scrollTarget` and optionally `dragPans` and `maxZoom` (see `src/qml/ViewerInput.qml`).
- **Licensing:** the project follows [REUSE](https://reuse.software/): new files need SPDX headers (`reuse lint` checks).
- **CI:** every push builds and tests on Fedora; `.github/workflows/ci.yml` lists the exact dependencies.

Please follow the [KDE Community Code of Conduct](https://kde.org/code-of-conduct/).

## Roadmap

- [x] Image, PDF, office, media, text/code, Markdown, archive, comic, 3D model and font viewers
- [x] Universal zoom and grab-to-pan
- [x] Flatpak, and one-click install from Dolphin's *Download New Services* (KDE Store)
- [ ] Flathub
- [ ] Polish: window resizes to fit images, pin the window size, settings

## How KPeek was made

KPeek's maintainer directed its design and tested it on real hardware, and wrote it together with an AI
coding assistant (Anthropic's Claude): most of the code, tests, packaging scripts and documentation were
drafted with its help and reviewed, tested and revised before being committed. Commits it contributed to
carry a `Co-Authored-By: Claude` line.

## License

GPL-2.0-or-later. Build files are BSD-2-Clause. See [`LICENSES/`](LICENSES/).

The screenshots show KDE Plasma's "Next" wallpaper (CC-BY-SA-4.0, by the KDE Visual Design Group).
