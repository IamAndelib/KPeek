// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Andelib Taref Srizon <andelibsriz@proton.me>

#include "officeconverter.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTimer>

using namespace std::chrono_literals;

namespace
{
constexpr auto ConversionTimeout = 60s;
const QString FlathubLibreOffice = QStringLiteral("org.libreoffice.LibreOffice");

bool insideFlatpak()
{
    return QFileInfo::exists(QStringLiteral("/.flatpak-info"));
}

bool runsSuccessfully(const QStringList &command)
{
    QProcess process;
    process.start(command.first(), command.mid(1));
    return process.waitForFinished(5000) && process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}
}

OfficeConverter::~OfficeConverter()
{
    stopProcess();
}

QStringList OfficeConverter::sofficeCommand()
{
    static const QStringList command = [] {
        if (insideFlatpak()) {
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
            return QStringList();
        }

        for (const QString &binary : {QStringLiteral("soffice"), QStringLiteral("libreoffice")}) {
            const QString path = QStandardPaths::findExecutable(binary);
            if (!path.isEmpty()) {
                return QStringList{path};
            }
        }
        const QString flatpak = QStandardPaths::findExecutable(QStringLiteral("flatpak"));
        if (!flatpak.isEmpty() && runsSuccessfully({flatpak, QStringLiteral("info"), FlathubLibreOffice})) {
            return QStringList{flatpak, QStringLiteral("run"), QStringLiteral("--command=soffice"), FlathubLibreOffice};
        }
        return QStringList();
    }();
    return command;
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

    stopProcess();
    m_pdfUrl.clear();

    const QFileInfo info(source.toLocalFile());
    if (!info.isFile()) {
        setStatus(Failed);
        return;
    }

    const QByteArray key =
        info.absoluteFilePath().toUtf8() + '\n' + QByteArray::number(info.size()) + '\n' + QByteArray::number(info.lastModified().toMSecsSinceEpoch());
    const QString cacheRoot = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/office");
    const QString outDir = cacheRoot + QLatin1Char('/') + QString::fromLatin1(QCryptographicHash::hash(key, QCryptographicHash::Sha1).toHex());
    const QString pdfPath = outDir + QLatin1Char('/') + info.completeBaseName() + QStringLiteral(".pdf");

    if (QFileInfo::exists(pdfPath)) {
        m_pdfUrl = QUrl::fromLocalFile(pdfPath);
        setStatus(Ready);
        return;
    }

    const QStringList command = sofficeCommand();
    if (command.isEmpty()) {
        setStatus(Unavailable);
        return;
    }

    QDir().mkpath(outDir);
    // A private profile lets the conversion run while the user has LibreOffice open.
    const QString profile = QUrl::fromLocalFile(cacheRoot + QStringLiteral("/lo-profile")).toString();

    m_process = new QProcess(this);
    m_process->setProgram(command.first());
    m_process->setArguments(command.mid(1)
                            + QStringList{QStringLiteral("-env:UserInstallation=") + profile,
                                          QStringLiteral("--headless"),
                                          QStringLiteral("--convert-to"),
                                          QStringLiteral("pdf"),
                                          QStringLiteral("--outdir"),
                                          outDir,
                                          info.absoluteFilePath()});
    connect(m_process, &QProcess::finished, this, [this, pdfPath, process = m_process.data()] {
        process->deleteLater();
        const bool ok = process->exitStatus() == QProcess::NormalExit && process->exitCode() == 0;
        if (!ok) {
            QFile::remove(pdfPath); // don't cache a half-written PDF from a killed conversion
        }
        if (ok && QFileInfo::exists(pdfPath)) {
            m_pdfUrl = QUrl::fromLocalFile(pdfPath);
            setStatus(Ready);
        } else {
            setStatus(Failed);
        }
    });
    connect(m_process, &QProcess::errorOccurred, this, [this, process = m_process.data()](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            process->deleteLater();
            setStatus(Failed);
        }
    });

    setStatus(Converting);
    m_process->start();
    QTimer::singleShot(ConversionTimeout, m_process, &QProcess::kill);
}

QUrl OfficeConverter::pdfUrl() const
{
    return m_pdfUrl;
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

void OfficeConverter::stopProcess()
{
    if (m_process) {
        m_process->disconnect(this);
        m_process->kill();
        m_process->waitForFinished(1000);
        delete m_process;
    }
}

#include "moc_officeconverter.cpp"
