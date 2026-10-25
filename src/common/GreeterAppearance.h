#ifndef SRC_COMMON_GREETERAPPEARANCE_H_
#define SRC_COMMON_GREETERAPPEARANCE_H_

#include <QString>

// Greeter wallpaper and cursor settings, shared by gxdm-greeter-classic,
// gxdm-lock-classic and gxdm-greeter-neo.
namespace GxdmGreeterAppearance {

QString stateFilePath();
QString cursorTheme(const QString& statePath = QString());
QString wallpaper(const QString& statePath = QString());
bool hasConfiguredWallpaper(const QString& statePath = QString());
QString lockWallpaperOverridePath(
  uint uid, const QString& dataDirectory = QString());
QString lockWallpaperOverride(
  uint uid, const QString& dataDirectory = QString());
QString systemLockWallpaperOverridePath(
  uint uid, const QString& stateDirectory = QString());
QString systemLockWallpaperOverride(
  uint uid, const QString& stateDirectory = QString());
QString gxdeDefaultWallpaper();
QString ddeLockDefaultWallpaper();

// The local path of a readable image given as a path or a file:// URL, or an
// empty string.
QString readableImage(const QString& path);

// The user's own lock wallpaper: the GXDM lock override, then Deepin
// Accounts, then freedesktop AccountsService. Empty when none has one.
QString userLockWallpaper(const QString& userName, uint uid);

}  // namespace GxdmGreeterAppearance

#endif  // SRC_COMMON_GREETERAPPEARANCE_H_
