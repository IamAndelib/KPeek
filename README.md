# KPeek

**Quick Look for Dolphin.** Select a file in KDE's file manager, press <kbd>Meta</kbd>+<kbd>Space</kbd>, and a preview pops up.
Use the arrow keys to move to the next file, and press the shortcut again to close the preview.

KPeek is to Dolphin what [PowerToys Peek](https://learn.microsoft.com/windows/powertoys/peek) is to Windows Explorer
and [Sushi](https://gitlab.gnome.org/GNOME/sushi) is to GNOME Files.

> **Status:** early development. The viewers below work; the Flatpak and the KDE Store listing are coming.

## What it previews

| Type | Preview |
|---|---|
| Images (JPEG, PNG, WebP, AVIF, JXL, HEIC, RAW, SVG…) and GIF/WebP animations | Fitted to the window |
| PDF | Every page, with scrolling, a page counter and zoom |
| Office documents (docx, odt, xlsx, ods, pptx, odp, doc, rtf…) | Rendered through LibreOffice, then cached |
| Video and audio | Starts playing immediately; seek bar, mute, and title/artist for audio |
| Source code and text | Syntax highlighting that follows your color scheme |
| Markdown | Rendered, with a source toggle |
| Archives (zip, tar.gz/xz/bz2/zst, 7z) | Contents tree with sizes |
| Fonts | Specimen at several sizes |
| Folders and everything else | Icon, type, size and modification date |

### Light on resources

- **Images** are decoded once at screen size, however many megapixels they have. Zooming in decodes only
  the visible part at full detail, so a 384-megapixel photo stays around 100–200 MB of memory.
- **Dolphin's thumbnails** are shown instantly as placeholders while the full image or video loads.
- **Video** is decoded by the graphics card where possible (VA-API). A large video in a codec the card
  probably can't decode (e.g. 4K AV1) waits for you to press Play instead of loading the processor.

### Keys and mouse

With several files selected, KPeek moves through those; with one, through its whole folder.

| Input | Action |
|---|---|
| <kbd>←</kbd> <kbd>→</kbd> | Previous / next file |
| Mouse wheel, <kbd>↑</kbd> <kbd>↓</kbd> <kbd>PgUp</kbd> <kbd>PgDn</kbd> <kbd>Home</kbd> <kbd>End</kbd> | Scroll |
| <kbd>Ctrl</kbd>+<kbd>+</kbd> <kbd>Ctrl</kbd>+<kbd>−</kbd> <kbd>Ctrl</kbd>+<kbd>0</kbd>, <kbd>Ctrl</kbd>+wheel | Zoom in, out, reset (images, PDFs, documents, text, fonts). The header shows the zoom level, with zoom buttons and a reset button. |
| <kbd>Ctrl</kbd>+drag | Grab and move the content; zoomed images also pan with a plain drag |
| <kbd>Enter</kbd> | Open in the default application |
| <kbd>Meta</kbd>+<kbd>Space</kbd>, <kbd>Space</kbd>, <kbd>Esc</kbd> | Close |
| <kbd>K</kbd> | Play/pause media |

## How it integrates with Dolphin

Dolphin has no plugin API for reading the current selection. KPeek therefore ships as a **service menu**: a "Peek" entry in
Dolphin's context menu that receives the selected files. Dolphin 26.04 and later let you put a keyboard shortcut on a service menu
action, so you can bind it under *Settings → Configure Keyboard Shortcuts → Peek*. The suggested shortcut is <kbd>Meta</kbd>+<kbd>Space</kbd>;
plain Space already toggles Dolphin's selection mode.

KPeek runs as a single instance. After the first preview it stays in the background for a few minutes, so the next one opens instantly.
Invoking it again on the file that's already showing closes the preview.

## Requirements

- KDE Plasma 6 with Dolphin ≥ 26.04 (needed for the keyboard shortcut; the context menu entry works with older versions)
- Qt ≥ 6.8, KDE Frameworks ≥ 6.10, Kirigami, Poppler (Qt 6), QtMultimedia, KSyntaxHighlighting, KArchive
- Optional: LibreOffice, from your distribution or Flathub, for office document previews

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

Then bind the shortcut as described above and restart Dolphin. After rebuilding, run `pkill -x kpeek`,
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
  zoom, drag and keyboard scrolling by exposing `zoom` and `scrollTarget` (see `src/qml/ViewerInput.qml`).
- **Licensing:** the project follows [REUSE](https://reuse.software/): new files need SPDX headers (`reuse lint` checks).
- **CI:** every push builds and tests on Fedora; `.github/workflows/ci.yml` lists the exact dependencies.

Please follow the [KDE Community Code of Conduct](https://kde.org/code-of-conduct/).

## Roadmap

- [x] Image, PDF, office, media, text/code, Markdown, archive and font viewers
- [x] Universal zoom and grab-to-pan
- [ ] Polish: window resizes to fit images, pin the window size, settings
- [ ] Flatpak on Flathub
- [ ] One-click install from Dolphin's *Get New → Dolphin Service Menus* (KDE Store), including the shortcut setup

## License

GPL-2.0-or-later. Build files are BSD-2-Clause. See [`LICENSES/`](LICENSES/).
