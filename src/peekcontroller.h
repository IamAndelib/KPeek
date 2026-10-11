// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#pragma once

#include <QDateTime>
#include <QFutureWatcher>
#include <QObject>
#include <QPointer>
#include <QQmlEngine>
#include <QSize>
#include <QUrl>

#include "filenavigator.h"

#include <KWindowShadow>

class QQuickWindow;

/**
 * Owns what is being previewed and the preview window's visibility.
 *
 * While a preview is open, KPeek is a single instance: further `kpeek <files>` invocations (from the
 * Dolphin service menu shortcut) are forwarded here through KDBusService. Invoking it again for what
 * is shown closes the preview, so one key both opens and closes it. Closing quits the process.
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
    Q_PROPERTY(QString sizeText READ sizeText NOTIFY sizeTextChanged)
    Q_PROPERTY(qint64 fileSize READ fileSize NOTIFY currentChanged)
    Q_PROPERTY(QDateTime modified READ modified NOTIFY currentChanged)
    Q_PROPERTY(int index READ index NOTIFY positionChanged)
    Q_PROPERTY(int count READ count NOTIFY positionChanged)
    /// Whether a preview is on screen. The viewer exists only while this is true.
    Q_PROPERTY(bool showing READ showing NOTIFY showingChanged)
    Q_PROPERTY(QSize imageSize READ imageSize NOTIFY currentChanged)
    Q_PROPERTY(bool animated READ animated NOTIFY currentChanged)
    /// The window's own scale factor. With fractional scaling (e.g. 125%) it differs from
    /// Screen.devicePixelRatio, which reports the output's rounded-up integer scale.
    Q_PROPERTY(qreal devicePixelRatio READ devicePixelRatio NOTIFY devicePixelRatioChanged)
    /// The largest texture the GPU takes, in pixels (known once the window has rendered).
    Q_PROPERTY(int maxTextureSize READ maxTextureSize NOTIFY maxTextureSizeChanged)
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
    qint64 fileSize() const;
    /// What loading the current 3D model reads: the file plus, for glTF, the files it names (any path at all).
    /// The largest possible value when one of those isn't a regular file (/dev/zero never ends).
    Q_INVOKABLE qint64 modelBytes() const;
    QDateTime modified() const;
    int index() const;
    int count() const;
    bool showing() const;
    QSize imageSize() const;
    bool animated() const;
    bool lowMemory() const;
    qreal devicePixelRatio() const;
    int maxTextureSize() const;
    QUrl thumbnail() const;

public Q_SLOTS:
    void next();
    void previous();
    void close();
    void openExternally();

Q_SIGNALS:
    void currentChanged();
    /// index or count changed without a new current file (the folder listing arrived).
    void positionChanged();
    void sizeTextChanged();
    void showingChanged();
    void devicePixelRatioChanged();
    void maxTextureSizeChanged();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void updateCurrent();
    void raiseWindow();

    FileNavigator m_navigator;
    QFutureWatcher<FileNavigator::Siblings> m_siblingsWatcher;
    QFutureWatcher<QString> m_folderSizeWatcher; // the item count, as text
    QPointer<QQuickWindow> m_window;
    KWindowShadow m_shadow;
    bool m_showing = false;

    QString m_viewer;
    QString m_mimeComment;
    QString m_iconName;
    bool m_isFolder = false;
    QString m_sizeText;
    qint64 m_fileSize = 0;
    QDateTime m_modified;
    QSize m_imageSize; ///< natural size, EXIF orientation applied; only set for the image viewer
    bool m_animated = false;
    QUrl m_thumbnail;
    int m_maxTextureSize = 8192; // a safe guess until the GPU says otherwise
};
