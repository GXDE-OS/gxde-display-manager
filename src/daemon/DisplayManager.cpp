/***************************************************************************
* Copyright (c) 2013 Abdurrahman AVCI <abdurrahmanavci@gmail.com>
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

#include "DisplayManager.h"

#include "Configuration.h"
#include "Constants.h"
#include "DaemonApp.h"
#include "SeatManager.h"

#include "displaymanageradaptor.h"
#include "gxdesystemadaptor.h"
#include "seatadaptor.h"
#include "sessionadaptor.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>

#include <pwd.h>
#include <unistd.h>

const QString DISPLAYMANAGER_SERVICE = QStringLiteral("org.freedesktop.DisplayManager");
const QString DISPLAYMANAGER_PATH = QStringLiteral("/org/freedesktop/DisplayManager");
const QString DISPLAYMANAGER_SEAT_PATH = QStringLiteral("/org/freedesktop/DisplayManager/Seat");
const QString DISPLAYMANAGER_SESSION_PATH = QStringLiteral("/org/freedesktop/DisplayManager/Session");

namespace {

const QString GXDE_DISPLAYMANAGER_SERVICE = QStringLiteral("top.gxde.DisplayManager");
const QString GXDE_DISPLAYMANAGER_PATH = QStringLiteral("/top/gxde/DisplayManager");
const QString GXDE_DEFAULT_WALLPAPER = QStringLiteral("/usr/share/backgrounds/default_background.jpg");
const QString DDE_LOCK_DEFAULT_WALLPAPER = QStringLiteral(":/theme/background/default_background.jpg");
constexpr qint64 MAX_WALLPAPER_SIZE = 128 * 1024 * 1024;

QString stateDirectory()
{
    const passwd *gxdmUser = getpwnam("gxdm");
    if (gxdmUser && gxdmUser->pw_dir)
        return QString::fromLocal8Bit(gxdmUser->pw_dir);
    return QStringLiteral(STATE_DIR);
}

bool cursorThemeExists(const QString &theme)
{
    static const QRegularExpression validName(
        QStringLiteral("^[A-Za-z0-9][A-Za-z0-9._+-]*$"));
    if (!validName.match(theme).hasMatch())
        return false;

    return !QStandardPaths::locate(
        QStandardPaths::GenericDataLocation,
        QStringLiteral("icons/%1/index.theme").arg(theme),
        QStandardPaths::LocateFile).isEmpty();
}

bool saveWallpaperPath(const QString &path)
{
    SDDM::stateConfig.Greeter.Wallpaper.set(path);
    SDDM::stateConfig.save();
    return true;
}

} // namespace

namespace SDDM {
    GxdeDisplayManager::GxdeDisplayManager(QObject *parent)
        : QObject(parent)
    {
        new SystemAdaptor(this);

        QDBusConnection connection = (daemonApp->testing())
            ? QDBusConnection::sessionBus()
            : QDBusConnection::systemBus();
        if (!connection.registerService(GXDE_DISPLAYMANAGER_SERVICE)) {
            qWarning() << "Failed to register" << GXDE_DISPLAYMANAGER_SERVICE
                       << connection.lastError();
            return;
        }
        if (!connection.registerObject(GXDE_DISPLAYMANAGER_PATH, this)) {
            qWarning() << "Failed to register" << GXDE_DISPLAYMANAGER_PATH
                       << connection.lastError();
        }
    }

    bool GxdeDisplayManager::SetCursor(const QString &theme)
    {
        if (!cursorThemeExists(theme))
            return false;

        stateConfig.Greeter.CursorTheme.set(theme);
        stateConfig.save();
        return true;
    }

    bool GxdeDisplayManager::SetWallpaperGXDEDefault()
    {
        return saveWallpaperPath(GXDE_DEFAULT_WALLPAPER);
    }

    bool GxdeDisplayManager::SetWallpaperDDELockDefault()
    {
        return saveWallpaperPath(DDE_LOCK_DEFAULT_WALLPAPER);
    }

    bool GxdeDisplayManager::SetWallpaper(
        const QDBusUnixFileDescriptor &wallpaper)
    {
        if (!wallpaper.isValid())
            return false;

        const int sourceFd = dup(wallpaper.fileDescriptor());
        if (sourceFd < 0)
            return false;

        QFile source;
        if (!source.open(sourceFd, QIODevice::ReadOnly,
                         QFileDevice::AutoCloseHandle)) {
            close(sourceFd);
            return false;
        }

        const QString directory = stateDirectory();
        if (!QDir().mkpath(directory))
            return false;

        const QString targetPath = directory
            + QStringLiteral("/greeter-wallpaper");
        QSaveFile target(targetPath);
        if (!target.open(QIODevice::WriteOnly))
            return false;

        qint64 total = 0;
        while (!source.atEnd()) {
            const QByteArray chunk = source.read(1024 * 1024);
            if (chunk.isEmpty() && source.error() != QFileDevice::NoError) {
                target.cancelWriting();
                return false;
            }
            total += chunk.size();
            if (total > MAX_WALLPAPER_SIZE || target.write(chunk) != chunk.size()) {
                target.cancelWriting();
                return false;
            }
        }

        if (total == 0 || !target.commit())
            return false;

        if (!QFile::setPermissions(targetPath,
                QFileDevice::ReadOwner | QFileDevice::WriteOwner
                    | QFileDevice::ReadGroup | QFileDevice::ReadOther)) {
            return false;
        }
        return saveWallpaperPath(targetPath);
    }

    DisplayManager::DisplayManager(QObject *parent) : QObject(parent) {
        // create adaptor
        new DisplayManagerAdaptor(this);

        // register object
        QDBusConnection connection = (daemonApp->testing()) ? QDBusConnection::sessionBus() : QDBusConnection::systemBus();
        connection.registerService(DISPLAYMANAGER_SERVICE);
        connection.registerObject(DISPLAYMANAGER_PATH, this);

        new GxdeDisplayManager(this);
    }

    QString DisplayManager::seatPath(const QString &seatName) {
        return DISPLAYMANAGER_SEAT_PATH + seatName.mid(4);
    }

    QString DisplayManager::sessionPath(const QString &sessionName) {
        return DISPLAYMANAGER_SESSION_PATH + sessionName.mid(7);
    }

    ObjectPathList DisplayManager::Seats() const {
        ObjectPathList seats;

        for (DisplayManagerSeat *seat: m_seats)
            seats << ObjectPath(seat->Path());

        return seats;
    }

    ObjectPathList DisplayManager::Sessions(DisplayManagerSeat *seat) const {
        ObjectPathList sessions;

        for (DisplayManagerSession *session: m_sessions)
            if (seat == nullptr || seat->Name() == session->Seat())
                sessions << ObjectPath(session->Path());

        return sessions;
    }

    void DisplayManager::AddSeat(const QString &name) {
        // create seat object
        DisplayManagerSeat *seat = new DisplayManagerSeat(name, this);

        // add to the list
        m_seats << seat;

        // emit signal
        emit SeatAdded(ObjectPath(seat->Path()));
    }

    void DisplayManager::RemoveSeat(const QString &name) {
        // find seat
        for (DisplayManagerSeat *seat: m_seats) {
            if (seat->Name() == name) {
                // remove from list
                m_seats.removeAll(seat);

                // get object path
                ObjectPath path = ObjectPath(seat->Path());

                // delete seat
                seat->deleteLater();

                // emit signal
                emit SeatRemoved(path);
            }
        }
    }

    void DisplayManager::AddSession(const QString &name, const QString &seat, const QString &user) {
        // create session object
        DisplayManagerSession *session = new DisplayManagerSession(name, seat, user, this);

        // add to the list
        m_sessions << session;

        // emit signal
        emit SessionAdded(ObjectPath(session->Path()));
    }

    void DisplayManager::RemoveSession(const QString &name) {
        // find session
        for (DisplayManagerSession *session: m_sessions) {
            if (session->Name() == name) {
                // remove from list
                m_sessions.removeAll(session);

                // get object path
                ObjectPath path = ObjectPath(session->Path());

                // delete session
                session->deleteLater();

                // emit signal
                emit SessionRemoved(path);
            }
        }
    }

    DisplayManagerSeat::DisplayManagerSeat(const QString &name, QObject *parent)
        : QObject(parent), m_name(name), m_path(DISPLAYMANAGER_SEAT_PATH + name.mid(4)) {
        // create adaptor
        new SeatAdaptor(this);

        // register object
        QDBusConnection connection = (daemonApp->testing()) ? QDBusConnection::sessionBus() : QDBusConnection::systemBus();
        connection.registerService(DISPLAYMANAGER_SERVICE);
        connection.registerObject(m_path, this);
    }

    const QString &DisplayManagerSeat::Name() const {
        return m_name;
    }

    const QString &DisplayManagerSeat::Path() const {
        return m_path;
    }

    void DisplayManagerSeat::SwitchToGreeter() {
        daemonApp->seatManager()->switchToGreeter(m_name);
    }

    void DisplayManagerSeat::SwitchToGuest(const QString &/*session*/) {
        // TODO: IMPLEMENT
    }

    void DisplayManagerSeat::SwitchToUser(const QString &/*user*/, const QString &/*session*/) {
        // TODO: IMPLEMENT
    }

    void DisplayManagerSeat::Lock() {
        // TODO: IMPLEMENT
    }

    ObjectPathList DisplayManagerSeat::Sessions() {
       return daemonApp->displayManager()->Sessions(this);
    }

    DisplayManagerSession::DisplayManagerSession(const QString &name, const QString &seat, const QString &user, QObject *parent)
        : QObject(parent), m_name(name), m_path(DISPLAYMANAGER_SESSION_PATH + name.mid(7)), m_seat(seat), m_user(user) {
        // create adaptor
        new SessionAdaptor(this);

        // register object
        QDBusConnection connection = (daemonApp->testing()) ? QDBusConnection::sessionBus() : QDBusConnection::systemBus();
        connection.registerService(DISPLAYMANAGER_SERVICE);
        connection.registerObject(m_path, this);
    }

    const QString &DisplayManagerSession::Name() const {
        return m_name;
    }

    const QString &DisplayManagerSession::Path() const {
        return m_path;
    }

    const QString &DisplayManagerSession::Seat() const {
        return m_seat;
    }

    void DisplayManagerSession::Lock() {
        // TODO: IMPLEMENT
    }

    ObjectPath DisplayManagerSession::SeatPath() const {
        return ObjectPath(DISPLAYMANAGER_SEAT_PATH + m_seat.mid(4));
    }

    const QString &DisplayManagerSession::User() const {
        return m_user;
    }
}
