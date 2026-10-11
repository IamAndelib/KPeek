<!--
SPDX-License-Identifier: CC0-1.0
SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>
-->
# Releasing KPeek

A release has two halves: GitHub builds and publishes it, then the KDE Store listing is updated so that
Dolphin offers the update to everyone who installed KPeek from there.

## 1. Prepare the version

In one commit:

- `CMakeLists.txt`: `project(kpeek VERSION x.y.z …)`
- `src/io.github.iamandelib.kpeek.metainfo.xml`: a new `<release version="x.y.z" date="YYYY-MM-DD">` at the top of
  `<releases>`, with a short description of what changed.

Push it and wait for CI to pass.

## 2. Tag it

```sh
git tag -a vx.y.z --cleanup=verbatim -F notes.txt   # first line "KPeek x.y.z", then the release notes
git push origin vx.y.z
```

The tag message (after its first line) becomes the release notes, in Markdown. `--cleanup=verbatim` keeps
`## Headings`: without it, git drops every line starting with `#` as a comment. The *Release* workflow checks that the version matches the two
files above, builds `kpeek.flatpak` and `kpeek-servicemenu-x.y.z.tar.gz`, and publishes the GitHub release with both
attached (about 20 minutes; follow it under *Actions*).

## 3. Update the KDE Store listing

1. Download `kpeek-servicemenu-x.y.z.tar.gz` from the new GitHub release.
2. On [store.kde.org](https://store.kde.org), open the KPeek product and click *Edit*.
3. Under *Files*, upload the new `kpeek-servicemenu-x.y.z.tar.gz`, set its version to `x.y.z`, and remove or
   archive the old file. Paste the release notes into the changelog field.
4. Set the product's version to `x.y.z` and save.

Dolphin then shows *Update* for KPeek in *Configure Dolphin → Context Menu → Download New Services*. Updating
re-runs the install script, which installs the newest GitHub release.

## Notes

- Users who installed the bundle by hand update by running `kpeek-servicemenu/install-it.sh` again.
- `flatpak update` doesn't update KPeek: a bundle installed from a file has no update source. The KDE Store
  listing (or Flathub, later) is the update channel.
- `packaging/flatpak/` builds the GitHub bundle. It is not the Flathub manifest: Flathub's policy requires that
  one to be written and submitted by a person, without AI tools.
