// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

// KPeek never runs Ghostscript on a previewed file. kimageformats' EPS plugin does, whenever Qt's format
// detection picks it from a file's content, which no MIME check can prevent: a PostScript file named photo.png
// is image/png, and comic pages, Markdown images and office thumbnails are detected the same way. These
// definitions take the place of libc's for the whole process (QProcess starts programs through them) and
// refuse to execute Ghostscript or the plugin's other helper, pdftops. Programs KPeek opens files with run in their own processes, unaffected.
// autotests/execguardtest.cpp fails if a Qt update starts programs some other way.

#include <cerrno>
#include <cstring>
#include <sys/syscall.h>
#include <unistd.h>

namespace
{
bool isGhostscript(const char *path)
{
    const char *slash = std::strrchr(path, '/');
    const char *name = slash ? slash + 1 : path;
    return std::strcmp(name, "gs") == 0 || std::strcmp(name, "pdftops") == 0;
}
}

// Only a raw system call here: this runs in a vfork()ed child.
extern "C" __attribute__((visibility("default"))) int execve(const char *path, char *const argv[], char *const envp[]) noexcept
{
    if (isGhostscript(path)) {
        errno = EACCES;
        return -1;
    }
    return int(syscall(SYS_execve, path, argv, envp));
}

extern "C" __attribute__((visibility("default"))) int execv(const char *path, char *const argv[]) noexcept
{
    return execve(path, argv, environ);
}
