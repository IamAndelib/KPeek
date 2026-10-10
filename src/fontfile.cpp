// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include "fontfile.h"

#include <QFontDatabase>
#include <QRawFont>

FontFile::~FontFile()
{
    unregister();
}

void FontFile::unregister()
{
    if (m_id >= 0) {
        QFontDatabase::removeApplicationFont(m_id);
        m_id = -1;
    }
}

QUrl FontFile::url() const
{
    return m_url;
}

void FontFile::setUrl(const QUrl &url)
{
    if (m_url == url) {
        return;
    }
    unregister();
    m_url = url;
    m_font = {};

    const QString path = url.toLocalFile();
    m_id = QFontDatabase::addApplicationFont(path);
    const QStringList families = m_id >= 0 ? QFontDatabase::applicationFontFamilies(m_id) : QStringList();
    if (!families.isEmpty()) {
        m_font.setFamily(families.first());
        // The file's own style ("Bold Italic"), so a family with several styles shows this one.
        m_font.setStyleName(QRawFont(path, 12).styleName());
    }
    Q_EMIT loaded();
}

QFont FontFile::font() const
{
    return m_font;
}

bool FontFile::failed() const
{
    return m_font.family().isEmpty();
}

#include "moc_fontfile.cpp"
