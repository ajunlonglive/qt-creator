// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "slog2inforunner.h"

#include "qnxdevice.h"
#include "qnxtr.h"

#include <projectexplorer/runconfigurationaspects.h>

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QRegularExpression>

using namespace ProjectExplorer;
using namespace Utils;

namespace Qnx::Internal {

Slog2InfoRunner::Slog2InfoRunner(RunControl *runControl)
    : RunWorker(runControl)
{
    setId("Slog2InfoRunner");
    m_applicationId = runControl->aspect<ExecutableAspect>()->executable.fileName();

    // See QTCREATORBUG-10712 for details.
    // We need to limit length of ApplicationId to 63 otherwise it would not match one in slog2info.
    m_applicationId.truncate(63);

    m_testProcess = new QtcProcess(this);
    connect(m_testProcess, &QtcProcess::done, this, &Slog2InfoRunner::handleTestProcessCompleted);

    m_launchDateTimeProcess = new QtcProcess(this);
    connect(m_launchDateTimeProcess, &QtcProcess::done, this, &Slog2InfoRunner::launchSlog2Info);

    m_logProcess = new QtcProcess(this);
    connect(m_logProcess, &QtcProcess::readyReadStandardOutput, this, &Slog2InfoRunner::readLogStandardOutput);
    connect(m_logProcess, &QtcProcess::readyReadStandardError, this, &Slog2InfoRunner::readLogStandardError);
    connect(m_logProcess, &QtcProcess::done, this, &Slog2InfoRunner::handleLogDone);
}

void Slog2InfoRunner::printMissingWarning()
{
    appendMessage(Tr::tr("Warning: \"slog2info\" is not found on the device, debug output not available."), ErrorMessageFormat);
}

void Slog2InfoRunner::start()
{
    m_testProcess->setCommand({device()->filePath("slog2info"), {}});
    m_testProcess->start();
    reportStarted();
}

void Slog2InfoRunner::stop()
{
    if (m_testProcess->state() == QProcess::Running)
        m_testProcess->kill();

    if (m_logProcess->state() == QProcess::Running) {
        m_logProcess->kill();
        processLog(true);
    }
    reportStopped();
}

bool Slog2InfoRunner::commandFound() const
{
    return m_found;
}

void Slog2InfoRunner::handleTestProcessCompleted()
{
    m_found = (m_testProcess->exitCode() == 0);
    if (m_found) {
        readLaunchTime();
    } else {
        QnxDevice::ConstPtr qnxDevice = device().dynamicCast<const QnxDevice>();
        if (qnxDevice->qnxVersion() > 0x060500) {
            printMissingWarning();
        }
    }
}

void Slog2InfoRunner::readLaunchTime()
{
    m_launchDateTimeProcess->setCommand({device()->filePath("date"),
                                         "+\"%d %H:%M:%S\"", CommandLine::Raw});
    m_launchDateTimeProcess->start();
}

void Slog2InfoRunner::launchSlog2Info()
{
    QTC_CHECK(!m_applicationId.isEmpty());
    QTC_CHECK(m_found);

    if (m_logProcess->state() == QProcess::Running)
        return;

    if (m_launchDateTimeProcess->error() != QProcess::UnknownError)
        return;

    m_launchDateTime = QDateTime::fromString(QString::fromLatin1(m_launchDateTimeProcess->readAllStandardOutput()).trimmed(),
                                             QString::fromLatin1("dd HH:mm:ss"));

    m_logProcess->setCommand({device()->filePath("slog2info"), {"-w"}});
    m_logProcess->start();
}

void Slog2InfoRunner::readLogStandardOutput()
{
    processLog(false);
}

void Slog2InfoRunner::processLog(bool force)
{
    QString input = QString::fromLatin1(m_logProcess->readAllStandardOutput());
    QStringList lines = input.split(QLatin1Char('\n'));
    if (lines.isEmpty())
        return;
    lines.first().prepend(m_remainingData);
    if (force)
        m_remainingData.clear();
    else
        m_remainingData = lines.takeLast();
    foreach (const QString &line, lines)
        processLogLine(line);
}

void Slog2InfoRunner::processLogLine(const QString &line)
{
    // The "(\\s+\\S+)?" represents a named buffer. If message has noname (aka empty) buffer
    // then the message might get cut for the first number in the message.
    // The "\\s+(\\b.*)?$" represents a space followed by a message. We are unable to determinate
    // how many spaces represent separators and how many are a part of the messages, so resulting
    // messages has all whitespaces at the beginning of the message trimmed.
    static QRegularExpression regexp(QLatin1String(
        "^[a-zA-Z]+\\s+([0-9]+ [0-9]+:[0-9]+:[0-9]+.[0-9]+)\\s+(\\S+)(\\s+(\\S+))?\\s+([0-9]+)\\s+(.*)?$"));

    const QRegularExpressionMatch match = regexp.match(line);
    if (!match.hasMatch())
        return;

    // Note: This is useless if/once slog2info -b displays only logs from recent launches
    if (!m_launchDateTime.isNull()) {
        // Check if logs are from the recent launch
        if (!m_currentLogs) {
            QDateTime dateTime = QDateTime::fromString(match.captured(1),
                                                       QLatin1String("dd HH:mm:ss.zzz"));
            m_currentLogs = dateTime >= m_launchDateTime;
            if (!m_currentLogs)
                return;
        }
    }

    QString applicationId = match.captured(2);
    if (!applicationId.startsWith(m_applicationId))
        return;

    QString bufferName = match.captured(4);
    int bufferId = match.captured(5).toInt();
    // filtering out standard BB10 messages
    if (bufferName == QLatin1String("default") && bufferId == 8900)
        return;

    appendMessage(match.captured(6).trimmed() + '\n', StdOutFormat);
}

void Slog2InfoRunner::readLogStandardError()
{
    appendMessage(QString::fromLatin1(m_logProcess->readAllStandardError()), StdErrFormat);
}

void Slog2InfoRunner::handleLogDone()
{
    if (m_logProcess->error() == QProcess::UnknownError)
        return;

    appendMessage(Tr::tr("Cannot show slog2info output. Error: %1")
                  .arg(m_logProcess->errorString()), StdErrFormat);
}

} // Qnx::Internal
