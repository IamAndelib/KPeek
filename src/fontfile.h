// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#pragma once

#include <QFont>
#include <QObject>
#include <QQmlEngine>
#include <QUrl>

/**
 * A font file registered with the application for as long as it is previewed.
 * QML's FontLoader never unregisters fonts, so a resident process browsing a fonts folder
 * would keep every one of them loaded.
 */
class FontFile : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QUrl url READ url WRITE setUrl NOTIFY loaded)
    Q_PROPERTY(QFont font READ font NOTIFY loaded)
    Q_PROPERTY(bool failed READ failed NOTIFY loaded)

public:
    using QObject::QObject;
    ~FontFile() override;

    QUrl url() const;
    void setUrl(const QUrl &url);
    QFont font() const;
    bool failed() const;

Q_SIGNALS:
    void loaded();

private:
    void unregister();

    QUrl m_url;
    QFont m_font;
    int m_id = -1;
};
