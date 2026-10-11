<!--
SPDX-License-Identifier: CC0-1.0
SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>
-->
# Publishing KPeek on the KDE Store

Everything the store's *Add Product* form asks for, in order. Files referred to here are in this folder unless a
path is given. For later versions, see `RELEASING.md` in the repository root.

## Before you start

Download `kpeek-servicemenu-1.0.6.tar.gz` from the
[GitHub release](https://github.com/IamAndelib/KPeek/releases/tag/v1.0.6). It is the file Dolphin downloads
and installs.

## The form

Open [store.kde.org/product/add](https://store.kde.org/product/add) (logged in as IamAndelib).

| Field | Value |
|---|---|
| Category | **Dolphin Service Menus** |
| Product name | `KPeek` |
| Version | `1.0.6` |
| Description | everything below the line in `description.md` |
| Tags | `original-product, kde, dolphin, servicemenu, quicklook, preview, flatpak, linux, gplv2` |
| License | GPLv2 or later (`GPL-2.0-or-later`) |
| Source (link to git repository) | `https://github.com/IamAndelib/KPeek` (the **Source** field, not Homepage) |
| Logo | `kpeek-512.png` |
| Gallery / screenshots | in order: `image.png`, `pdf.png`, `code.png` from `docs/screenshots/` |
| Changelog | `changelog.txt` |

### Files step

| Field | Value |
|---|---|
| File | `kpeek-servicemenu-1.0.6.tar.gz` |
| Version | `1.0.6` |
| Description | `Install from Dolphin: Configure Dolphin → Context Menu → Download New Services` |

Publish.

## After publishing

1. Wait a few minutes, then in Dolphin open *Settings → Configure Dolphin… → Context Menu → Download New
   Services…*, search for **KPeek** and check the listing (description, screenshots, version).
2. Add the store link to the GitHub repository: on GitHub, open the repository's *About* settings (gear icon
   next to "About") and set *Website* to the product's store.kde.org address.
