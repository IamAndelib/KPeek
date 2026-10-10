// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#pragma once

#include <QDateTime>
#include <QObject>
#include <QPointer>
#include <QQmlEngine>
#include <QSize>
#include <QTimer>
#include <QUrl>

#include "filenavigator.h"

class QQuickWindow;

/**
 * Owns what is being previewed and the preview window's visibility.
 *
 * KPeek is a single resident instance: each `kpeek <files>` invocation (from the
 * Dolphin service menu shortcut) is forwarded here through KDBusService. Invoking it
 * again for what is already shown closes the preview, so one key both opens and closes it.
 */
class PeekController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QUrl url READ url NOTIFY currentChanged)
    Q_PROPERTY(QString fileName READ fileName NOTIFY currentChanged)
    Q_PROPERTY(QString viewer READ viewer NOTIFY currentChanged)
    Q_PROPERTY(QString mimeComment READ mimeComment NOTIFY currentChanged)
    Q_PROPERTY(QString iconName READ iconName NOTIFY currentChanged)
    Q_PROPERTY(bool isFolder READ isFolder NOTIFY currentChanged)
    Q_PROPERTY(QString sizeText READ sizeText NOTIFY currentChanged)
    Q_PROPERTY(QDateTime modified READ modified NOTIFY currentChanged)
    Q_PROPERTY(int index READ index NOTIFY currentChanged)
    Q_PROPERTY(int count READ count NOTIFY currentChanged)
    /// Whether a preview is on screen. The viewer exists only while this is true.
    Q_PROPERTY(bool showing READ showing NOTIFY showingChanged)
    Q_PROPERTY(QSize imageSize READ imageSize NOTIFY currentChanged)
    Q_PROPERTY(bool animated READ animated NOTIFY currentChanged)
    /// The window's own scale factor. With fractional scaling (e.g. 125%) it differs from
    /// Screen.devicePixelRatio, which reports the output's rounded-up integer scale.
    Q_PROPERTY(qreal devicePixelRatio READ devicePixelRatio NOTIFY devicePixelRatioChanged)
    /// See isLowMemorySystem(): lower caps for image decoding.
    Q_PROPERTY(bool lowMemory READ lowMemory CONSTANT)
    /// Dolphin's cached thumbnail of the file, shown instantly while the real content loads; may be empty.
    Q_PROPERTY(QUrl thumbnail READ thumbnail NOTIFY currentChanged)

public:
    // Not default-constructible on purpose: QML must use create() and get the one instance from main().
    explicit PeekController(QObject *parent);

    static PeekController *create(QQmlEngine *, QJSEngine *);

    void setWindow(QQuickWindow *window);

    /// Entry point for every invocation. @p toggle closes the preview when @p urls is what's shown.
    void showFiles(const QList<QUrl> &urls, bool toggle = true);

    QUrl url() const;
    QString fileName() const;
    QString viewer() const;
    QString mimeComment() const;
    QString iconName() const;
    bool isFolder() const;
    QString sizeText() const;
    QDateTime modified() const;
    int index() const;
    int count() const;
    bool showing() const;
    QSize imageSize() const;
    bool animated() const;
    bool lowMemory() const;
    qreal devicePixelRatio() const;
    QUrl thumbnail() const;

public Q_SLOTS:
    void next();
    void previous();
    void close();
    void openExternally();

Q_SIGNALS:
    void currentChanged();
    void showingChanged();
    void devicePixelRatioChanged();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void updateCurrent();
    void raiseWindow();

    FileNavigator m_navigator;
    QPointer<QQuickWindow> m_window;
    QTimer m_idleQuitTimer;
    bool m_showing = false;

    QString m_viewer;
    QString m_mimeComment;
    QString m_iconName;
    bool m_isFolder = false;
    QString m_sizeText;
    QDateTime m_modified;
    QSize m_imageSize; ///< natural size, EXIF orientation applied; only set for the image viewer
    bool m_animated = false;
    QUrl m_thumbnail;
};
