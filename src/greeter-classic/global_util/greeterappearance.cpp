#include "greeterappearance.h"

#include <pwd.h>

#include <QFile>
#include <QSettings>

namespace {

QString resolvedStatePath(const QString& statePath) {
  return statePath.isEmpty() ? GxdmGreeterAppearance::stateFilePath()
                             : statePath;
}

bool isReadableFile(const QString& path) {
  QFile file(path);
  return file.open(QIODevice::ReadOnly);
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

QString gxdeDefaultWallpaper() {
  return QStringLiteral("/usr/share/backgrounds/default_background.jpg");
}

QString ddeLockDefaultWallpaper() {
  return QStringLiteral(":/theme/background/default_background.jpg");
}

}  // namespace GxdmGreeterAppearance
