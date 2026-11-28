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
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QSysInfo>

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

    const QStringList command = QProcess::splitCommand(cmd);
    const QString program =
        command.isEmpty() ? QString() : QFileInfo(command.constFirst()).fileName();
    const bool hisiCompositor = program == QLatin1String("flakewm") ||
                                program == QLatin1String("gxde-wlcom");

    // Kirin 9000c (Maleoon 910) only has the hisi EGL/gbm stack. flakewm and
    // gxde-wlcom know how to drive it when the compat package's libraries and
    // LD_PRELOAD shim are present. Other greeter compositors cannot use hisi
    // EGL on the server side, so only enable this for those two.
    if (QSysInfo::kernelVersion().contains(QLatin1String("9000c"))) {
        if (hisiCompositor && PrepareHisi9000cEnvironment()) {
            m_hisi9000c = true;
            m_environment.insert(QStringLiteral("GXDM_HISI_9000C"), QStringLiteral("1"));
            m_environment.insert(QStringLiteral("GXDM_GREETER_ANIMATIONS"), QStringLiteral("1"));
        } else {
            // No working hardware path: run the QML greeter with Qt's software
            // backend and make it skip animations entirely.
            m_environment.insert(QStringLiteral("GXDM_GREETER_ANIMATIONS"), QStringLiteral("0"));
            m_environment.insert(QStringLiteral("QT_QUICK_BACKEND"), QStringLiteral("software"));
            qWarning() << "Kirin 9000c: no hisi greeter acceleration available;"
                          " running the greeter with software rendering";
        }
    }

    // The compositor must not inherit a stale Wayland/X11 display, otherwise
    // flakewm's backend autocreate selects the nested backend instead of DRM.
    QProcessEnvironment compositorEnvironment = m_environment;
    compositorEnvironment.remove(QStringLiteral("WAYLAND_DISPLAY"));
    compositorEnvironment.remove(QStringLiteral("WAYLAND_SOCKET"));
    compositorEnvironment.remove(QStringLiteral("DISPLAY"));

    return startProcess(cmd, &m_serverProcess, compositorEnvironment);
}

bool WaylandHelper::PrepareHisi9000cEnvironment()
{
    const QDir compat(QStringLiteral("/usr/lib/gxde-hisi-9000c-gpu-compat"));
    if (!QFileInfo::exists(compat.filePath(QStringLiteral("server-egl/libEGL.so.1"))) ||
        !QFileInfo::exists(compat.filePath(QStringLiteral("libhisi_gbm_shim.so")))) {
        qWarning() << "Kirin 9000c detected, but gxde-hisi-9000c-gpu-compat is not installed";
        return false;
    }

    const QString libraryPath = compat.filePath(QStringLiteral("server-egl"));
    const QString shim = compat.filePath(QStringLiteral("libhisi_gbm_shim.so"));
    QStringList libraryPaths = m_environment.value(QStringLiteral("LD_LIBRARY_PATH"))
                                   .split(QLatin1Char(':'), Qt::SkipEmptyParts);
    if (!libraryPaths.contains(libraryPath)) {
        libraryPaths.prepend(libraryPath);
    }
    m_environment.insert(QStringLiteral("LD_LIBRARY_PATH"),
                         libraryPaths.join(QLatin1Char(':')));

    QStringList preloads = m_environment.value(QStringLiteral("LD_PRELOAD"))
                               .split(QLatin1Char(':'), Qt::SkipEmptyParts);
    if (!preloads.contains(shim)) {
        preloads.prepend(shim);
    }
    m_environment.insert(QStringLiteral("LD_PRELOAD"),
                         preloads.join(QLatin1Char(':')));
    m_environment.insert(QStringLiteral("WLR_BACKENDS"), QStringLiteral("drm,libinput"));
    m_environment.insert(QStringLiteral("WLR_EGL_NO_MODIFIERS"), QStringLiteral("1"));
    m_environment.insert(QStringLiteral("KYWC_EGL_NO_MODIFIERS"), QStringLiteral("1"));
    m_environment.insert(QStringLiteral("WLR_XWAYLAND"),
                         compat.filePath(QStringLiteral("xwayland-mesa")));

    qInfo() << "Kirin 9000c: enabling hisi hardware acceleration for the greeter";
    return true;
}

void WaylandHelper::InspectCompositorOutput(const QByteArray &output)
{
    if (m_hardwareRendererFailed || output.isEmpty()) {
        return;
    }
    if (output.contains("Failed to create a hardware renderer") ||
        output.contains("Could not initialize renderer") ||
        output.contains("Failed to initialize EGL")) {
        m_hardwareRendererFailed = true;
    }
}

void WaylandHelper::DrainCompositorOutput()
{
    if (!m_serverProcess) {
        return;
    }
    const QByteArray output = m_serverProcess->readAllStandardOutput();
    if (!output.isEmpty()) {
        InspectCompositorOutput(output);
        qInfo() << output;
    }
}

void WaylandHelper::PrepareGreeterEnvironment()
{
    if (!m_hisi9000c) {
        return;
    }
    DrainCompositorOutput();
    if (m_hardwareRendererFailed) {
        m_environment.insert(QStringLiteral("GXDM_GREETER_ANIMATIONS"), QStringLiteral("0"));
        qWarning() << "The greeter compositor has no hardware renderer;"
                      " disabling greeter animations";
    }
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

bool WaylandHelper::startProcess(const QString &cmd, QProcess **p,
                                const QProcessEnvironment &environment)
{
    auto *process = new CompositorProcess(this);
    process->setProcessEnvironment(environment);
    process->setInputChannelMode(QProcess::ForwardedInputChannel);
    connect(process, &QProcess::readyReadStandardError, this, [this, process] {
        const QByteArray output = process->readAllStandardError();
        InspectCompositorOutput(output);
        qWarning() << output;
    });
    connect(process, &QProcess::readyReadStandardOutput, this, [this, process] {
        const QByteArray output = process->readAllStandardOutput();
        InspectCompositorOutput(output);
        qInfo() << output;
    });
    qDebug() << "Starting Wayland process" << cmd << environment.value(QStringLiteral("USER"));
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
    // A stale DISPLAY/WAYLAND_DISPLAY must never leak into the greeter; the
    // real WAYLAND_DISPLAY is inserted once the compositor socket is known.
    m_environment.remove(QStringLiteral("WAYLAND_DISPLAY"));
    m_environment.remove(QStringLiteral("WAYLAND_SOCKET"));
    m_environment.remove(QStringLiteral("DISPLAY"));

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
        PrepareGreeterEnvironment();
        m_environment.insert(QStringLiteral("WAYLAND_DISPLAY"), m_watcher->socketName());
        m_greeterProcess->setProcessEnvironment(m_environment);
        m_greeterProcess->start();
    } else if (m_watcher->status() == WaylandSocketWatcher::Failed) {
        Q_EMIT failed();
    } else {
        connect(m_watcher, &WaylandSocketWatcher::failed, this, &WaylandHelper::failed);
        connect(m_watcher, &WaylandSocketWatcher::started, this, [this] {
            m_watcher->stop();
            PrepareGreeterEnvironment();
            m_environment.insert(QStringLiteral("WAYLAND_DISPLAY"), m_watcher->socketName());
            m_greeterProcess->setProcessEnvironment(m_environment);
            m_greeterProcess->start();
        });
    }
}

} // namespace SDDM
