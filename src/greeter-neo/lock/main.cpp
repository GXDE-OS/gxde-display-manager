/*
 * Copyright (C) 2015 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     sbw <sbw@sbw.so>
 *             kirigaya <kirigaya@mkacg.com>
 *             Hualet <mr.asianwang@gmail.com>
 *
 * Maintainer: sbw <sbw@sbw.so>
 *             kirigaya <kirigaya@mkacg.com>
 *             Hualet <mr.asianwang@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <DLog>

#include "lockframe.h"
#include "lockshortcutmanager.h"
#include "dbus/dbuslockfrontservice.h"
#include "dbus/dbuslockagent.h"
// PATCHS
#include "freedesktopscreensaver.h"
// PATCHE
#include "multiscreenmanager.h"

#include "lockcontent.h"
#include "lockworker.h"
#include "sessionbasemodel.h"
#include "propertygroup.h"
#include "systemdefaults.h"

#include <QLabel>
#include <QScreen>
#include <QWindow>
#include <dapplication.h>
#include <QDBusInterface>
#include <QTimer>
// PATCHS
// Qt6: removed `#include <QDesktopWidget>` (QDesktopWidget no longer exists)
// PATCHE

DCORE_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

namespace {

const QString kGxdeDisplayManagerService =
    QStringLiteral("top.gxde.DisplayManager");
const QString kGxdeDisplayManagerPath =
    QStringLiteral("/top/gxde/DisplayManager");
const QString kGxdeDisplayManagerInterface =
    QStringLiteral("top.gxde.DisplayManager");

bool shouldUseX11(int argc, char *argv[])
{
    QByteArray requestedPlatform = qgetenv("QT_QPA_PLATFORM");
    for (int i = 1; i + 1 < argc; ++i) {
        if (QByteArray(argv[i]) == "-platform") {
            requestedPlatform = argv[i + 1];
        }
    }

    return requestedPlatform.startsWith("xcb")
        || requestedPlatform.startsWith("dxcb")
        || (requestedPlatform.isEmpty()
            && !qEnvironmentVariableIsSet("WAYLAND_DISPLAY"));
}

} // namespace

int main(int argc, char *argv[])
{
    if (shouldUseX11(argc, argv))
        DApplication::loadDXcbPlugin();

    DApplication app(argc, argv);
    app.setOrganizationName("deepin");
    app.setApplicationName("gxdm-lock-neo");
    app.setApplicationVersion("2015.1.0");

    DLogManager::registerConsoleAppender();
    DLogManager::registerFileAppender();

    QTranslator translator;
    translator.load("/usr/share/gxde-session-ui/translations/gxde-session-ui_" + QLocale::system().name());
    app.installTranslator(&translator);

    QCommandLineParser cmdParser;
    cmdParser.addHelpOption();
    cmdParser.addVersionOption();

    QCommandLineOption backend(QStringList() << "d" << "daemon", "start to daemon mode");
    cmdParser.addOption(backend);
    QCommandLineOption switchUser(QStringList() << "s" << "switch", "show user switch");
    cmdParser.addOption(switchUser);
    QCommandLineOption autoEnroll(
        QStringLiteral("auto-enroll"),
        QStringLiteral("run on GXDE and enroll the lock shortcut"));
    cmdParser.addOption(autoEnroll);
    cmdParser.process(app);

    const bool runDaemon = cmdParser.isSet(backend);
    const bool showUserList = cmdParser.isSet(switchUser);
    const bool isGxde = GxdmSystemDefaults::isGxdeOperatingSystem();

    if (cmdParser.isSet(autoEnroll) && !isGxde)
        return 0;

    if (!app.setSingleInstance(
            QStringLiteral("gxdm-lock-neo"), DApplication::UserScope)) {
        if (!runDaemon) {
            if (showUserList) {
                QDBusInterface lockFront(
                    DBUS_NAME,
                    DBUS_PATH,
                    QStringLiteral("com.deepin.dde.lockFront"),
                    QDBusConnection::sessionBus());
                lockFront.asyncCall(QStringLiteral("ShowUserList"));
            } else {
                QDBusInterface displayManager(
                    kGxdeDisplayManagerService,
                    kGxdeDisplayManagerPath,
                    kGxdeDisplayManagerInterface,
                    QDBusConnection::sessionBus());
                displayManager.asyncCall(QStringLiteral("Show"));
            }
        }
        return 0;
    }

    SessionBaseModel *model = new SessionBaseModel(SessionBaseModel::AuthType::LockType);
    LockWorker *worker = new LockWorker(model); //
    PropertyGroup *property_group = new PropertyGroup(worker);

    property_group->addProperty("contentVisible");

    auto createFrame = [&] (QScreen *screen) -> QWidget* {
        LockFrame *lockFrame = new LockFrame(model);
        lockFrame->setScreen(screen);
        property_group->addObject(lockFrame);
        QObject::connect(lockFrame, &LockFrame::requestSwitchToUser, worker, &LockWorker::switchToUser);
        QObject::connect(lockFrame, &LockFrame::requestAuthUser, worker, &LockWorker::authUser);
        QObject::connect(model, &SessionBaseModel::visibleChanged, lockFrame, &LockFrame::setVisible);
        QObject::connect(model, &SessionBaseModel::showUserList, lockFrame, &LockFrame::showUserList);
        QObject::connect(lockFrame, &LockFrame::requestSetLayout, worker, &LockWorker::setLayout);
        QObject::connect(lockFrame, &LockFrame::requestEnableHotzone, worker, &LockWorker::enableZoneDetected, Qt::UniqueConnection);
        QObject::connect(lockFrame, &LockFrame::destroyed, property_group, &PropertyGroup::removeObject);
        lockFrame->setVisible(model->isShow());
        return lockFrame;
    };

    MultiScreenManager multi_screen_manager;
    multi_screen_manager.register_for_mutil_screen(createFrame);

    QObject::connect(model, &SessionBaseModel::visibleChanged, &multi_screen_manager, &MultiScreenManager::startRaiseContentFrame);

    DBusLockAgent agent;
    agent.setModel(model);
    DBusLockFrontService service(&agent);
    Q_UNUSED(service);
    // PATCHS
    // Also expose org.freedesktop.ScreenSaver so cross-desktop "Lock Screen"
    // actions (XFCE/GNOME/Cinnamon/MATE probe this name) trigger the GXDM locker.
    FreedesktopScreenSaver screenSaver(&agent, model);
    Q_UNUSED(screenSaver);
    // PATCHE

    LockShortcutManager shortcutManager(model);
    GxdeDisplayManagerService displayManagerService(&shortcutManager);
    Q_UNUSED(displayManagerService)

    QDBusConnection conn = QDBusConnection::sessionBus();
    if (!conn.registerService(kGxdeDisplayManagerService)
        || !conn.registerObject(
            kGxdeDisplayManagerPath,
            &shortcutManager,
            QDBusConnection::ExportAdaptors)) {
        qWarning() << "Failed to register"
                   << kGxdeDisplayManagerService << conn.lastError();
        return 1;
    }

    // Compatibility names are best-effort. The GXDE interface and shortcut
    // remain usable even when another locker already owns one of these names.
    if (conn.registerService(DBUS_NAME)) {
        conn.registerObject(
            QStringLiteral("/com/deepin/dde/lockFront"), &agent);
    } else {
        qInfo() << "com.deepin.dde.lockFront is already owned.";
    }

    if (conn.registerService(QStringLiteral("org.freedesktop.ScreenSaver"))) {
        conn.registerObject(
            QStringLiteral("/org/freedesktop/ScreenSaver"), &agent);
        conn.registerObject(QStringLiteral("/ScreenSaver"), &agent);
    }

    if (isGxde)
        QTimer::singleShot(0, &shortcutManager,
            [&shortcutManager] { shortcutManager.tryEnroll(true); });

    if (!runDaemon) {
        if (showUserList) {
            emit model->showUserList();
        } else {
            model->setIsShow(true);
        }
    }

    app.exec();

    return 0;
}
