// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibsriz@proton.me>

#pragma once

#include <QList>
#include <QObject>
#include <QQmlEngine>
#include <QQuickAsyncImageProvider>
#include <QSizeF>
#include <QUrl>

/**
 * Page count and page sizes of a PDF, for laying out the page list in QML.
 * The pages themselves are rendered on demand by PdfPageProvider ("image://kpeekpdf/<page>/<path>").
 */
class PdfDocument : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QUrl url READ url WRITE setUrl NOTIFY urlChanged)
    Q_PROPERTY(int pageCount READ pageCount NOTIFY loaded)
    Q_PROPERTY(bool failed READ failed NOTIFY loaded)
    Q_PROPERTY(bool locked READ locked NOTIFY loaded)

public:
    using QObject::QObject;

    QUrl url() const;
    void setUrl(const QUrl &url);

    int pageCount() const;
    bool failed() const;
    bool locked() const;

    /// Page size in points (1/72 inch).
    Q_INVOKABLE QSizeF pageSize(int page) const;
    /// Image provider source for @p page.
    Q_INVOKABLE QString pageSource(int page) const;

Q_SIGNALS:
    void urlChanged();
    void loaded();

private:
    QUrl m_url;
    QList<QSizeF> m_pageSizes;
    bool m_failed = false;
    bool m_locked = false;
};

class PdfPageProvider : public QQuickAsyncImageProvider
{
public:
    static constexpr auto Id = "kpeekpdf";

    QQuickImageResponse *requestImageResponse(const QString &id, const QSize &requestedSize) override;
};
