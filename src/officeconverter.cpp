// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibtarefsrizon@gmail.com>

#include "officeconverter.h"
#include "workerpool.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QProcess>
#include <QStandardPaths>
#include <QTimer>
#include <QtConcurrentRun>

using namespace std::chrono_literals;

namespace
{
constexpr auto ConversionTimeout = 2min;
constexpr auto CacheLifetime = std::chrono::hours(24 * 30);
const QString FlathubLibreOffice = QStringLiteral("org.libreoffice.LibreOffice");

QString cacheRoot()
{
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/office");
}

bool runsSuccessfully(const QStringList &command)
{
    QProcess process;
    process.start(command.first(), command.mid(1));
    return process.waitForFinished(5000) && process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

/**
 * The one LibreOffice conversion queue. Lives as long as the application, independent of
 * the viewers, so a conversion outlives the OfficeConverter that requested it.
 */
class ConversionQueue : public QObject
{
    Q_OBJECT

public:
    static ConversionQueue *instance()
    {
        static auto *queue = new ConversionQueue(qApp);
        return queue;
    }

    /// Converts @p sourcePath to @p pdfPath, unless that is already queued or running.
    void request(const QString &sourcePath, const QString &pdfPath)
    {
        if (m_running == pdfPath || std::any_of(m_pending.cbegin(), m_pending.cend(), [&](const Job &job) {
                return job.pdfPath == pdfPath;
            })) {
            return;
        }
        m_pending.append({sourcePath, pdfPath});
        startNext();
    }

    /// Drops a conversion nobody waits for any more, unless it has already started.
    void forget(const QString &pdfPath)
    {
        m_pending.removeIf([&](const Job &job) {
            return job.pdfPath == pdfPath;
        });
    }

    bool isUnavailable() const
    {
        return m_commandKnown && m_command.isEmpty();
    }

Q_SIGNALS:
    void finished(const QString &pdfPath, OfficeConverter::Status status);

private:
    struct Job {
        QString sourcePath;
        QString pdfPath;
    };

    explicit ConversionQueue(QObject *parent)
        : QObject(parent)
    {
        // Find LibreOffice and tidy the cache off the GUI thread; probing can take seconds inside a Flatpak.
        auto *watcher = new QFutureWatcher<QStringList>(this);
        connect(watcher, &QFutureWatcher<QStringList>::finished, this, [this, watcher] {
            m_command = watcher->result();
            m_commandKnown = true;
            watcher->deleteLater();
            startNext();
        });
        watcher->setFuture(QtConcurrent::run(workerPool(), [] {
            OfficeConverter::pruneCache();
            return OfficeConverter::sofficeCommand();
        }));

        // Let a conversion in progress finish cleanly when the app quits, rather than orphaning LibreOffice,
        // and start no new one: waitForFinished() delivers finished(), which would otherwise call startNext().
        connect(qApp, &QCoreApplication::aboutToQuit, this, [this] {
            m_quitting = true;
            m_pending.clear();
            if (m_process) {
                m_process->terminate();
                m_process->waitForFinished(3000);
            }
        });
    }

    void startNext()
    {
        if (m_quitting || !m_commandKnown || m_process || m_pending.isEmpty()) {
            return;
        }
        const Job job = m_pending.takeFirst();
        if (m_command.isEmpty()) {
            Q_EMIT finished(job.pdfPath, OfficeConverter::Unavailable);
            startNext();
            return;
        }

        // Convert into a temporary folder and move the result into place, so an interrupted
        // conversion can never leave a half-written PDF that later looks cached.
        const QString outDir = QFileInfo(job.pdfPath).absolutePath();
        const QString tmpDir = outDir + QStringLiteral(".tmp");
        QDir(tmpDir).removeRecursively();
        QDir().mkpath(tmpDir);
        // A private profile lets conversions run while the user has LibreOffice open.
        const QString profile = QUrl::fromLocalFile(cacheRoot() + QStringLiteral("/lo-profile")).toString();

        m_running = job.pdfPath;
        m_process = new QProcess(this);
        m_process->setProgram(m_command.first());
        m_process->setArguments(m_command.mid(1)
                                + QStringList{QStringLiteral("-env:UserInstallation=") + profile,
                                              QStringLiteral("--headless"),
                                              QStringLiteral("--convert-to"),
                                              QStringLiteral("pdf"),
                                              QStringLiteral("--outdir"),
                                              tmpDir,
                                              job.sourcePath});
        connect(m_process, &QProcess::finished, this, [this, job, outDir, tmpDir](int exitCode, QProcess::ExitStatus exitStatus) {
            const QString converted = tmpDir + QLatin1Char('/') + QFileInfo(job.pdfPath).fileName();
            bool ok = exitStatus == QProcess::NormalExit && exitCode == 0 && QFileInfo::exists(converted);
            if (ok) {
                QDir().mkpath(outDir);
                ok = QFile::rename(converted, job.pdfPath);
            }
            QDir(tmpDir).removeRecursively();
            done(job.pdfPath, ok ? OfficeConverter::Ready : OfficeConverter::Failed);
        });
        connect(m_process, &QProcess::errorOccurred, this, [this, job](QProcess::ProcessError error) {
            if (error == QProcess::FailedToStart) {
                done(job.pdfPath, OfficeConverter::Failed);
            }
        });
        QTimer::singleShot(ConversionTimeout, m_process, &QProcess::terminate);
        m_process->start();
    }

    void done(const QString &pdfPath, OfficeConverter::Status status)
    {
        m_process->deleteLater();
        m_process = nullptr;
        m_running.clear();
        Q_EMIT finished(pdfPath, status);
        startNext();
    }

    QList<Job> m_pending;
    QString m_running;
    QProcess *m_process = nullptr;
    QStringList m_command;
    bool m_commandKnown = false;
    bool m_quitting = false;
};
}

QStringList OfficeConverter::sofficeCommand()
{
    if (QFileInfo::exists(QStringLiteral("/.flatpak-info"))) {
        // The sandbox has no LibreOffice; use the host's.
        const QStringList host = {QStringLiteral("flatpak-spawn"), QStringLiteral("--host")};
        for (const QString &binary : {QStringLiteral("soffice"), QStringLiteral("libreoffice")}) {
            if (runsSuccessfully(host + QStringList{QStringLiteral("which"), binary})) {
                return host + QStringList{binary};
            }
        }
        if (runsSuccessfully(host + QStringList{QStringLiteral("flatpak"), QStringLiteral("info"), FlathubLibreOffice})) {
            return host + QStringList{QStringLiteral("flatpak"), QStringLiteral("run"), QStringLiteral("--command=soffice"), FlathubLibreOffice};
        }
        return {};
    }

    for (const QString &binary : {QStringLiteral("soffice"), QStringLiteral("libreoffice")}) {
        const QString path = QStandardPaths::findExecutable(binary);
        if (!path.isEmpty()) {
            return {path};
        }
    }
    const QString flatpak = QStandardPaths::findExecutable(QStringLiteral("flatpak"));
    if (!flatpak.isEmpty() && runsSuccessfully({flatpak, QStringLiteral("info"), FlathubLibreOffice})) {
        return {flatpak, QStringLiteral("run"), QStringLiteral("--command=soffice"), FlathubLibreOffice};
    }
    return {};
}

void OfficeConverter::pruneCache()
{
    const QDateTime cutoff = QDateTime::currentDateTime().addSecs(-std::chrono::duration_cast<std::chrono::seconds>(CacheLifetime).count());
    QDirIterator it(cacheRoot(), QDir::Dirs | QDir::NoDotAndDotDot);
    while (it.hasNext()) {
        const QFileInfo dir = it.nextFileInfo();
        if (dir.fileName() != QLatin1String("lo-profile") && dir.lastModified() < cutoff) {
            QDir(dir.absoluteFilePath()).removeRecursively();
        }
    }
}

OfficeConverter::OfficeConverter(QObject *parent)
    : QObject(parent)
{
    connect(ConversionQueue::instance(), &ConversionQueue::finished, this, [this](const QString &pdfPath, Status status) {
        if (m_status == Converting && pdfPath == m_pdfPath) {
            setStatus(status);
        }
    });
}

OfficeConverter::~OfficeConverter()
{
    if (m_status == Converting) {
        ConversionQueue::instance()->forget(m_pdfPath);
    }
}

QUrl OfficeConverter::source() const
{
    return m_source;
}

void OfficeConverter::setSource(const QUrl &source)
{
    if (m_source == source) {
        return;
    }
    m_source = source;
    Q_EMIT sourceChanged();

    const QFileInfo info(source.toLocalFile());
    if (!info.isFile()) {
        m_pdfPath.clear();
        setStatus(Failed);
        return;
    }

    const QByteArray key =
        info.absoluteFilePath().toUtf8() + '\n' + QByteArray::number(info.size()) + '\n' + QByteArray::number(info.lastModified().toMSecsSinceEpoch());
    const QString outDir = cacheRoot() + QLatin1Char('/') + QString::fromLatin1(QCryptographicHash::hash(key, QCryptographicHash::Sha1).toHex());
    m_pdfPath = outDir + QLatin1Char('/') + info.completeBaseName() + QStringLiteral(".pdf");

    if (QFileInfo::exists(m_pdfPath)) {
        setStatus(Ready);
        return;
    }

    auto *queue = ConversionQueue::instance();
    if (queue->isUnavailable()) {
        setStatus(Unavailable);
        return;
    }
    setStatus(Converting);
    queue->request(info.absoluteFilePath(), m_pdfPath);
}

QUrl OfficeConverter::pdfUrl() const
{
    return m_status == Ready ? QUrl::fromLocalFile(m_pdfPath) : QUrl();
}

OfficeConverter::Status OfficeConverter::status() const
{
    return m_status;
}

void OfficeConverter::setStatus(Status status)
{
    m_status = status;
    Q_EMIT statusChanged();
}

#include "moc_officeconverter.cpp"
#include "officeconverter.moc"
