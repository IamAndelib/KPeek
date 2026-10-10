// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#pragma once

#include <QMimeType>
#include <QString>

namespace ViewerResolver
{
/// Viewer id for a MIME type; matches a QML file name in qml/viewers/ ("Image" → ImageViewer.qml).
QString viewerFor(const QMimeType &mime);

/// Like viewerFor(), but files of unknown type that look like plain text get the text viewer.
QString viewerForFile(const QString &path, const QMimeType &mime);

/// True when @p data has no NUL bytes and decodes as UTF-8.
bool looksLikeText(const QByteArray &data);
}
