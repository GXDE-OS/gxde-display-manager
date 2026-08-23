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

QString lockWallpaperOverrideFileName(uint uid)
{
    return QStringLiteral("lock-wallpaper-override-%1").arg(uid);
}

QString lockWallpaperOverridePath(uint uid)
{
    return stateDirectory() + QLatin1Char('/')
        + lockWallpaperOverrideFileName(uid);
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

QString saveWallpaperDescriptor(const QDBusUnixFileDescriptor &wallpaper,
        const QString &fileName)
{
    if (!wallpaper.isValid())
        return QString();

    const int sourceFd = dup(wallpaper.fileDescriptor());
    if (sourceFd < 0)
        return QString();

    QFile source;
    if (!source.open(sourceFd, QIODevice::ReadOnly,
                     QFileDevice::AutoCloseHandle)) {
        close(sourceFd);
        return QString();
    }

    const QString directory = stateDirectory();
    if (!QDir().mkpath(directory))
        return QString();

    const QString targetPath = directory + QLatin1Char('/') + fileName;
    QSaveFile target(targetPath);
    if (!target.open(QIODevice::WriteOnly))
        return QString();

    qint64 total = 0;
    while (!source.atEnd()) {
        const QByteArray chunk = source.read(1024 * 1024);
        if (chunk.isEmpty() && source.error() != QFileDevice::NoError) {
            target.cancelWriting();
            return QString();
        }
        total += chunk.size();
        if (total > MAX_WALLPAPER_SIZE
                || target.write(chunk) != chunk.size()) {
            target.cancelWriting();
            return QString();
        }
    }

    if (total == 0 || !target.commit())
        return QString();

    if (!QFile::setPermissions(targetPath,
            QFileDevice::ReadOwner | QFileDevice::WriteOwner
                | QFileDevice::ReadGroup | QFileDevice::ReadOther)) {
        return QString();
    }

    return targetPath;
}

QString normalizedDisplayServer(const QString& displayServer) {
    const QString normalized = displayServer.trimmed().toLower();
    if (normalized == QLatin1String("wayland")
            || normalized == QLatin1String("x11")
            || normalized == QLatin1String("x11-user")) {
        return normalized;
    }
    return QString();
}

QString configLineWithoutComment(const QString& line) {
    const int commentPosition = line.indexOf(QLatin1Char('#'));
    return (commentPosition < 0 ? line : line.left(commentPosition)).trimmed();
}

QString sectionName(const QString& line) {
    const QString configLine = configLineWithoutComment(line);
    if (!configLine.startsWith(QLatin1Char('['))
            || !configLine.endsWith(QLatin1Char(']'))) {
        return QString();
    }
    return configLine.mid(1, configLine.size() - 2).trimmed();
}

QString assignmentName(const QString& line) {
    const QString configLine = configLineWithoutComment(line);
    const int separator = configLine.indexOf(QLatin1Char('='));
    if (separator < 0)
        return QString();
    return configLine.left(separator).trimmed();
}

QString displayServerLine(const QString& originalLine,
        const QString& displayServer) {
    const int commentPosition = originalLine.indexOf(QLatin1Char('#'));
    const QString comment = commentPosition < 0
        ? QString() : originalLine.mid(commentPosition).trimmed();
    return QStringLiteral("DisplayServer=%1%2").arg(displayServer,
        comment.isEmpty() ? QString() : QStringLiteral(" ") + comment);
}

bool saveDisplayServerConfig(const QString& displayServer) {
    const QString configPath = QStringLiteral(CONFIG_FILE);
    const QFileInfo configInfo(configPath);
    if (!QDir().mkpath(configInfo.absolutePath())) {
        return false;
    }

    QStringList lines;
    QFile currentConfig(configPath);
    if (currentConfig.exists()) {
        if (!currentConfig.open(QIODevice::ReadOnly)) {
            return false;
        }

        const QString content = QString::fromUtf8(currentConfig.readAll());
        lines = content.split(QLatin1Char('\n'));
        if (content.endsWith(QLatin1Char('\n')))
            lines.removeLast();
    }

    bool inGeneral = true;
    bool sawExplicitGeneral = false;
    bool wroteDisplayServer = false;
    int firstNonGeneralSection = -1;
    int generalInsertIndex = lines.size();

    for (int i = 0; i < lines.size(); ++i) {
        const QString section = sectionName(lines.at(i));
        if (!section.isEmpty()) {
            if (section == QLatin1String(IMPLICIT_SECTION)) {
                inGeneral = true;
                sawExplicitGeneral = true;
                generalInsertIndex = i + 1;
            } else {
                if (inGeneral && firstNonGeneralSection < 0)
                    firstNonGeneralSection = i;
                if (inGeneral)
                    generalInsertIndex = i;
                inGeneral = false;
            }
            continue;
        }

        if (!inGeneral)
            continue;

        if (assignmentName(lines.at(i)) == QLatin1String("DisplayServer")) {
            lines[i] = displayServerLine(lines.at(i), displayServer);
            wroteDisplayServer = true;
        } else {
            generalInsertIndex = i + 1;
        }
    }

    if (!wroteDisplayServer) {
        const QString configLine = displayServerLine(QString(), displayServer);
        if (lines.isEmpty()) {
            lines << QStringLiteral("[%1]")
                .arg(QStringLiteral(IMPLICIT_SECTION))
                << configLine;
        } else if (sawExplicitGeneral || firstNonGeneralSection >= 0) {
            const int insertIndex = sawExplicitGeneral
                ? generalInsertIndex
                : firstNonGeneralSection;
            lines.insert(insertIndex, configLine);
        } else {
            lines << configLine;
        }
    }

    QSaveFile savedConfig(configPath);
    if (!savedConfig.open(QIODevice::WriteOnly))
        return false;

    const QByteArray content = (lines.join(QLatin1Char('\n'))
        + QLatin1Char('\n')).toUtf8();
    if (savedConfig.write(content) != content.size())
        return false;

    if (!savedConfig.commit())
        return false;

    QFile::setPermissions(configPath,
        QFileDevice::ReadOwner | QFileDevice::WriteOwner
        | QFileDevice::ReadGroup | QFileDevice::ReadOther);
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
        if (!saveWallpaperPath(GXDE_DEFAULT_WALLPAPER))
            return false;
        emit WallpaperChanged(GXDE_DEFAULT_WALLPAPER);
        return true;
    }

    bool GxdeDisplayManager::SetWallpaperDDELockDefault()
    {
        if (!saveWallpaperPath(DDE_LOCK_DEFAULT_WALLPAPER))
            return false;
        emit WallpaperChanged(DDE_LOCK_DEFAULT_WALLPAPER);
        return true;
    }

    bool GxdeDisplayManager::SetWallpaper(
        const QDBusUnixFileDescriptor &wallpaper)
    {
        const QString targetPath = saveWallpaperDescriptor(wallpaper,
            QStringLiteral("greeter-wallpaper"));
        if (targetPath.isEmpty())
            return false;
        if (!saveWallpaperPath(targetPath))
            return false;
        emit WallpaperChanged(targetPath);
        return true;
    }

    bool GxdeDisplayManager::ClearWallpaper()
    {
        // 写入空值表示“未配置全局壁纸”，登录界面将回退到
        // 单用户的锁屏壁纸 / 多用户的默认登录壁纸。
        stateConfig.Greeter.Wallpaper.set(QString());
        stateConfig.save();
        // 顺手清理 SetWallpaper 保存的副本文件（尽力而为，失败不影响结果）
        QFile::remove(stateDirectory() + QStringLiteral("/greeter-wallpaper"));
        emit WallpaperChanged(QString());
        return true;
    }

    bool GxdeDisplayManager::SetLockWallpaperOverride(
        uint uid,
        const QDBusUnixFileDescriptor &wallpaper)
    {
        const QString targetPath = saveWallpaperDescriptor(wallpaper,
            lockWallpaperOverrideFileName(uid));
        if (targetPath.isEmpty())
            return false;
        emit LockWallpaperOverrideChanged(uid, targetPath);
        return true;
    }

    bool GxdeDisplayManager::ClearLockWallpaperOverride(uint uid)
    {
        const QString path = lockWallpaperOverridePath(uid);
        if (QFileInfo::exists(path) && !QFile::remove(path))
            return false;
        emit LockWallpaperOverrideChanged(uid, QString());
        return true;
    }

    QString GxdeDisplayManager::LockWallpaperOverride(uint uid) const
    {
        const QString path = lockWallpaperOverridePath(uid);
        const QFileInfo file(path);
        return file.isFile() && file.isReadable() ? path : QString();
    }

    bool GxdeDisplayManager::SetGreeterDisplayServer(
            const QString& displayServer) {
        const QString normalized = normalizedDisplayServer(displayServer);
        if (normalized.isEmpty()) {
            return false;
        }

        if (!saveDisplayServerConfig(normalized)) {
            return false;
        }

        mainConfig.DisplayServer.set(normalized);
        return true;
    }

    QString GxdeDisplayManager::GreeterDisplayServer() const {
        mainConfig.load();
        const QString normalized =
            normalizedDisplayServer(mainConfig.DisplayServer.get());
        return normalized.isEmpty() ? QStringLiteral("x11") : normalized;
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
