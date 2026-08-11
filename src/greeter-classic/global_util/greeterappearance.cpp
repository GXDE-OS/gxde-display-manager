#include "greeterappearance.h"

#include <pwd.h>

#include <QFile>
#include <QFileInfo>
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

QString lockWallpaperOverride(uint uid, const QString& stateDirectory) {
  const QString directory = stateDirectory.isEmpty()
    ? QFileInfo(stateFilePath()).absolutePath()
    : stateDirectory;
  const QString configured = directory + QStringLiteral(
    "/lock-wallpaper-override-%1").arg(uid);

  return isReadableFile(configured) ? configured : QString();
}

QString gxdeDefaultWallpaper() {
  return QStringLiteral("/usr/share/backgrounds/default_background.jpg");
}

QString ddeLockDefaultWallpaper() {
  return QStringLiteral(":/theme/background/default_background.jpg");
}

}  // namespace GxdmGreeterAppearance
