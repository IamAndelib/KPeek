// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#pragma once

#include <optional>

/**
 * Hands the command line to a KPeek that is already running and returns its exit status, or
 * nothing when there is none (or anything goes wrong): then this process becomes the preview.
 *
 * KDBusService does the same, but only after QApplication has connected to the display and loaded
 * the platform theme: 40–70 ms spent on every key press just to pass a file name along. This is the
 * same D-Bus call (org.kde.KDBusService.CommandLine) made before any of that, with plain libdbus.
 */
std::optional<int> forwardToRunningInstance(int argc, char **argv, const char *serviceName);
