/***************************************************************************
* Copyright (c) 2021 Aleix Pol Gonzalez <aleixpol@kde.org>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the
* Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
***************************************************************************/

#include <QCoreApplication>
#include <QFile>
#include <QStandardPaths>

#include "Configuration.h"

#include "waylandhelper.h"
#include "waylandsocketwatcher.h"
#include "VirtualTerminal.h"

#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#ifdef Q_OS_LINUX
#include <sys/prctl.h>
#endif

namespace SDDM {

namespace {
// Keep the greeter compositor's children separate from the helper and from
// user sessions, so a launcher exiting first cannot leave its WM behind.
class CompositorProcess : public QProcess
{
public:
    explicit CompositorProcess(QObject *parent) : QProcess(parent)
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        const pid_t parentPid = getpid();
        setChildProcessModifier([parentPid] { prepareChild(parentPid); });
#endif
    }

private:
    static void prepareChild(pid_t parentPid)
    {
        if (setpgid(0, 0) != 0)
            _exit(127);
#ifdef Q_OS_LINUX
        // Also release the GPU if our supervising helper is forcibly killed.
        if (prctl(PR_SET_PDEATHSIG, SIGKILL) != 0 || getppid() != parentPid)
            _exit(127);
#else
        Q_UNUSED(parentPid);
#endif
    }
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    const pid_t m_parentPid = getpid();
    void setupChildProcess() override { prepareChild(m_parentPid); }
#endif
};
}

WaylandHelper::WaylandHelper(QObject *parent)
    : QObject(parent)
    , m_environment(QProcessEnvironment::systemEnvironment())
    , m_watcher(new WaylandSocketWatcher(this))
{
}

WaylandHelper::~WaylandHelper()
{
    stop();
}

bool WaylandHelper::startCompositor(const QString &cmd)
{
    m_watcher->start();
    return startProcess(cmd, &m_serverProcess);
}

static void stopProcess(QProcess *&process, qint64 processGroup = -1)
{
    if (processGroup > 0)
        ::kill(-static_cast<pid_t>(processGroup), SIGTERM);

    if (process && process->state() != QProcess::NotRunning) {
        qInfo() << "Stopping..." << process->program();
        process->terminate();
        if (!process->waitForFinished(5000)) {
            process->kill();
            if (!process->waitForFinished(5000))
                qWarning() << "Could not finish Wayland process" << process->program();
        }
    }
    // The launcher may already have exited while descendants are still alive.
    if (processGroup > 0)
        ::kill(-static_cast<pid_t>(processGroup), SIGKILL);
    if (process)
        process->deleteLater();
    process = nullptr;
}

void WaylandHelper::stop()
{
    if (m_stopping)
        return;
    m_stopping = true;
    m_watcher->stop();
    stopProcess(m_greeterProcess);
    stopProcess(m_serverProcess, m_serverProcessGroup);
    m_serverProcessGroup = -1;
}

bool WaylandHelper::startProcess(const QString &cmd, QProcess **p)
{
    auto *process = new CompositorProcess(this);
    process->setProcessEnvironment(m_environment);
    process->setInputChannelMode(QProcess::ForwardedInputChannel);
    connect(process, &QProcess::readyReadStandardError, this, [process] {
        qWarning() << process->readAllStandardError();
    });
    connect(process, &QProcess::readyReadStandardOutput, this, [process] {
        qInfo() << process->readAllStandardOutput();
    });
    qDebug() << "Starting Wayland process" << cmd << m_environment.value(QStringLiteral("USER"));
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            process, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        qDebug() << "wayland compositor finished" << exitCode << exitStatus;
        if (!m_stopping)
            Q_EMIT failed();
    });

    auto args = QProcess::splitCommand(cmd);
    const auto program = args.takeFirst();
    process->start(program, args);
    if (!process->waitForStarted(10000)) {
        qWarning("Failed to start \"%s\": %s",
                 qPrintable(cmd),
                 qPrintable(process->errorString()));
        return false;
    }

    if (p)
        *p = process;
    m_serverProcessGroup = process->processId();

    qDebug() << "started succesfully" << cmd;
    return true;
}

void WaylandHelper::startGreeter(const QString &cmd)
{
    auto args = QProcess::splitCommand(cmd);

    m_greeterProcess = new QProcess(this);
    m_greeterProcess->setProgram(args.takeFirst());
    m_greeterProcess->setArguments(args);
    connect(m_greeterProcess, &QProcess::readyReadStandardError, this, [this] {
        qWarning() << m_greeterProcess->readAllStandardError();
    });
    connect(m_greeterProcess, &QProcess::readyReadStandardOutput, this, [this] {
        qInfo() << m_greeterProcess->readAllStandardOutput();
    });
    connect(m_greeterProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            m_greeterProcess, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        qDebug() << "wayland greeter finished" << exitCode << exitStatus;
        if (m_stopping)
            return;

        if (exitCode != 0 || exitStatus != QProcess::NormalExit)
            Q_EMIT failed();
        else
            QCoreApplication::instance()->quit();
    });
    if (m_watcher->status() == WaylandSocketWatcher::Started) {
        m_environment.insert(QStringLiteral("WAYLAND_DISPLAY"), m_watcher->socketName());
        m_greeterProcess->setProcessEnvironment(m_environment);
        m_greeterProcess->start();
    } else if (m_watcher->status() == WaylandSocketWatcher::Failed) {
        Q_EMIT failed();
    } else {
        connect(m_watcher, &WaylandSocketWatcher::failed, this, &WaylandHelper::failed);
        connect(m_watcher, &WaylandSocketWatcher::started, this, [this] {
            m_watcher->stop();
            m_environment.insert(QStringLiteral("WAYLAND_DISPLAY"), m_watcher->socketName());
            m_greeterProcess->setProcessEnvironment(m_environment);
            m_greeterProcess->start();
        });
    }
}

} // namespace SDDM
