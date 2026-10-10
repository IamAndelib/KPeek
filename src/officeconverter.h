// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#pragma once

#include <QObject>
#include <QPointer>
#include <QProcess>
#include <QQmlEngine>
#include <QUrl>

/**
 * Converts office documents to PDF with LibreOffice so the PDF viewer can show them,
 * the same approach GNOME Sushi takes. Uses a native LibreOffice or the Flathub one, and
 * reaches the host's through flatpak-spawn when KPeek itself runs as a Flatpak.
 * Results are cached by path, size and modification time.
 */
class OfficeConverter : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QUrl pdfUrl READ pdfUrl NOTIFY statusChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)

public:
    enum Status {
        Idle,
        Converting,
        Ready,
        Unavailable, ///< LibreOffice isn't installed
        Failed,
    };
    Q_ENUM(Status)

    using QObject::QObject;
    ~OfficeConverter() override;

    QUrl source() const;
    void setSource(const QUrl &source);

    QUrl pdfUrl() const;
    Status status() const;

    /// Command prefix that runs LibreOffice's soffice, or empty when none is installed.
    static QStringList sofficeCommand();

Q_SIGNALS:
    void sourceChanged();
    void statusChanged();

private:
    void setStatus(Status status);
    void stopProcess();

    QUrl m_source;
    QUrl m_pdfUrl;
    Status m_status = Idle;
    QPointer<QProcess> m_process;
};
