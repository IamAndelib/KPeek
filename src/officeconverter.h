// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QUrl>

/**
 * Shows office documents by converting them to PDF with LibreOffice, the same approach
 * GNOME Sushi takes. Uses a native LibreOffice or the Flathub one, reaching the host's
 * through flatpak-spawn when KPeek itself runs as a Flatpak.
 *
 * Conversions run one at a time in a shared queue and are cached by path, size and
 * modification time. A started conversion keeps running when the user moves on, so coming
 * back is instant and LibreOffice is never killed halfway (which would leave it orphaned);
 * queued ones nobody waits for any more are dropped.
 */
class OfficeConverter : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QUrl pdfUrl READ pdfUrl NOTIFY statusChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    /// Image source for the preview picture stored in the document itself, or empty.
    Q_PROPERTY(QString embeddedPreview READ embeddedPreview NOTIFY statusChanged)
    /// In the Flatpak, documents are never converted: only the embedded preview is shown.
    Q_PROPERTY(bool sandboxed READ sandboxed CONSTANT)

public:
    enum Status {
        Idle,
        Converting,
        Ready,
        Unavailable, ///< LibreOffice isn't installed
        Failed,
    };
    Q_ENUM(Status)

    explicit OfficeConverter(QObject *parent = nullptr);
    ~OfficeConverter() override;

    QUrl source() const;
    void setSource(const QUrl &source);

    QUrl pdfUrl() const;
    Status status() const;
    QString embeddedPreview() const;
    bool sandboxed() const;

    /// Command prefix that runs LibreOffice's soffice, or empty when none is installed. Blocking; call off the GUI thread.
    static QStringList sofficeCommand();

    /// Deletes cached conversions older than a month. Blocking; call off the GUI thread.
    static void pruneCache();

Q_SIGNALS:
    void sourceChanged();
    void statusChanged();

private:
    void setStatus(Status status);

    QUrl m_source;
    QString m_pdfPath;
    QString m_embeddedPreview;
    Status m_status = Idle;
};
