#include "GreeterAppearance.h"

#include <pwd.h>
#include <unistd.h>

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusObjectPath>
#include <QDBusReply>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QSettings>
#include <QStringList>
#include <QUrl>

namespace {

QString resolvedStatePath(const QString& statePath) {
  return statePath.isEmpty() ? GxdmGreeterAppearance::stateFilePath()
                             : statePath;
}

bool isReadableFile(const QString& path) {
  QFile file(path);
  return file.open(QIODevice::ReadOnly);
}

// GreeterBackground, falling back to BackgroundFile, of an Accounts user
// object; both the Deepin daemon and the deepin/GXDE accounts-service fork
// expose them.
QString accountsUserBackground(const QString& service, const QString& path,
    const QString& interface) {
  QDBusInterface user(service, path, interface,
    QDBusConnection::systemBus());
  if (!user.isValid()) return QString();

  const QString background = GxdmGreeterAppearance::readableImage(
    user.property("GreeterBackground").toString());
  return background.isEmpty()
    ? GxdmGreeterAppearance::readableImage(
        user.property("BackgroundFile").toString())
    : background;
}

QString deepinAccountsLockBackground(const QString& userName) {
  QDBusInterface accounts(QStringLiteral("com.deepin.daemon.Accounts"),
    QStringLiteral("/com/deepin/daemon/Accounts"),
    QStringLiteral("com.deepin.daemon.Accounts"),
    QDBusConnection::systemBus());
  if (!accounts.isValid()) return QString();

  const QDBusReply<QString> userPath = accounts.call(
    QStringLiteral("FindUserByName"), userName);
  if (!userPath.isValid() || userPath.value().isEmpty()) return QString();

  return accountsUserBackground(QStringLiteral("com.deepin.daemon.Accounts"),
    userPath.value(), QStringLiteral("com.deepin.daemon.Accounts.User"));
}

QString accountsServiceLockBackground(const QString& userName) {
  // org.freedesktop.Accounts already runs before any user session, so it is
  // reachable from the greeter even without com.deepin.daemon.Accounts.
  QDBusInterface accounts(QStringLiteral("org.freedesktop.Accounts"),
    QStringLiteral("/org/freedesktop/Accounts"),
    QStringLiteral("org.freedesktop.Accounts"),
    QDBusConnection::systemBus());
  if (!accounts.isValid()) return QString();

  const QDBusReply<QDBusObjectPath> userPath = accounts.call(
    QStringLiteral("FindUserByName"), userName);
  if (!userPath.isValid() || userPath.value().path().isEmpty()) {
    return QString();
  }

  return accountsUserBackground(QStringLiteral("org.freedesktop.Accounts"),
    userPath.value().path(), QStringLiteral("org.freedesktop.Accounts.User"));
}

}  // namespace

namespace GxdmGreeterAppearance {

QString stateFilePath() {
  const passwd* gxdmUser = getpwnam("gxdm");
  const QString stateDirectory = gxdmUser && gxdmUser->pw_dir
                                   ? QString::fromLocal8Bit(gxdmUser->pw_dir)
                                   : QStringLiteral("/var/lib/gxdm");
  return stateDirectory + QStringLiteral("/state.conf");
}

QString cursorTheme(const QString& statePath) {
  const QSettings settings(resolvedStatePath(statePath), QSettings::IniFormat);
  return settings.value(QStringLiteral("Greeter/CursorTheme"))
    .toString()
    .trimmed();
}

QString wallpaper(const QString& statePath) {
  const QSettings settings(resolvedStatePath(statePath), QSettings::IniFormat);
  const QString configured =
    settings.value(QStringLiteral("Greeter/Wallpaper"), gxdeDefaultWallpaper())
      .toString();

  if (isReadableFile(configured)) return configured;
  if (isReadableFile(gxdeDefaultWallpaper())) return gxdeDefaultWallpaper();
  return ddeLockDefaultWallpaper();
}

bool hasConfiguredWallpaper(const QString& statePath) {
  // state.conf 中是否显式写入过 [Greeter] Wallpaper。
  // 值为空（ClearWallpaper 写入 "Wallpaper="）视为未配置，
  // 未写入时调用方应回退到用户自己的壁纸，而不是全局默认图。
  const QSettings settings(resolvedStatePath(statePath), QSettings::IniFormat);
  const QString configured = settings.value(
    QStringLiteral("Greeter/Wallpaper"), QString()).toString().trimmed();
  return !configured.isEmpty();
}

QString lockWallpaperOverridePath(uint uid, const QString& dataDirectory) {
  QString directory = dataDirectory;
  if (directory.isEmpty()) {
    const passwd* user = getpwuid(uid);
    if (!user || !user->pw_dir) return QString();
    directory = QString::fromLocal8Bit(user->pw_dir)
      + QStringLiteral("/.local/share/gxdm");
  }
  return directory + QStringLiteral("/lock-wallpaper-override");
}

QString lockWallpaperOverride(uint uid, const QString& dataDirectory) {
  const QString configured = lockWallpaperOverridePath(uid, dataDirectory);

  return isReadableFile(configured) ? configured : QString();
}

QString systemLockWallpaperOverridePath(uint uid,
    const QString& stateDirectory) {
  QString directory = stateDirectory;
  if (directory.isEmpty())
    directory = QFileInfo(stateFilePath()).absolutePath();
  return directory + QStringLiteral("/lock-wallpaper-override-%1").arg(uid);
}

QString systemLockWallpaperOverride(uint uid, const QString& stateDirectory) {
  const QString configured =
    systemLockWallpaperOverridePath(uid, stateDirectory);

  return isReadableFile(configured) ? configured : QString();
}

QString gxdeDefaultWallpaper() {
  return QStringLiteral("/usr/share/backgrounds/default_background.jpg");
}

QString ddeLockDefaultWallpaper() {
  return QStringLiteral(":/theme/background/default_background.jpg");
}

QString readableImage(const QString& path) {
  const QUrl url(path);
  const QString localPath = url.isLocalFile() ? url.toLocalFile() : path;
  const QFileInfo file(localPath);
  if (!file.isFile() || !file.isReadable()) return QString();

  QImageReader reader(localPath);
  return reader.canRead() ? localPath : QString();
}

QString userLockWallpaper(const QString& userName, uint uid) {
  // 1. A GXDM lock override belongs to one user and never affects the
  //    global greeter wallpaper. It is written to ~/.local/share/gxdm (for
  //    the in-session locker) and ~gxdm/lock-wallpaper-override-<uid>
  //    (readable by the greeter even when the home directory is not).
  //    The in-session locker prefers the freshly written user copy; the
  //    greeter can only rely on the system one.
  QStringList overrides = {systemLockWallpaperOverride(uid),
                           lockWallpaperOverride(uid)};
  if (uid == getuid()) overrides.swapItemsAt(0, 1);
  for (const QString& path : overrides) {
    const QString background = readableImage(path);
    if (!background.isEmpty()) {
      qInfo() << "(Frontend) Lock background: Using user override for"
        << userName << background;
      return background;
    }
  }

  // 2. Deepin Accounts, the interface the classic lightdm greeter used for
  //    the login wallpaper. It is a system service, so it also works before
  //    the user session starts.
  const QString accountsBackground = deepinAccountsLockBackground(userName);
  if (!accountsBackground.isEmpty()) {
    qInfo() << "(Frontend) Lock background: Using Deepin Accounts for"
      << userName << accountsBackground;
    return accountsBackground;
  }

  // 3. Freedesktop AccountsService for systems without the Deepin daemon.
  const QString freedesktopBackground =
    accountsServiceLockBackground(userName);
  if (!freedesktopBackground.isEmpty()) {
    qInfo() << "(Frontend) Lock background: Using AccountsService for"
      << userName << freedesktopBackground;
  }
  return freedesktopBackground;
}

}  // namespace GxdmGreeterAppearance
