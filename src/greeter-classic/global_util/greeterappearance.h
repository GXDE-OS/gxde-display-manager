#ifndef SRC_GREETER_NEO_GLOBAL_UTIL_GREETERAPPEARANCE_H_
#define SRC_GREETER_NEO_GLOBAL_UTIL_GREETERAPPEARANCE_H_

#include <QString>

namespace GxdmGreeterAppearance {

QString stateFilePath();
QString cursorTheme(const QString& statePath = QString());
QString wallpaper(const QString& statePath = QString());
QString lockWallpaperOverride(
  uint uid, const QString& stateDirectory = QString());
QString gxdeDefaultWallpaper();
QString ddeLockDefaultWallpaper();

}  // namespace GxdmGreeterAppearance

#endif  // SRC_GREETER_NEO_GLOBAL_UTIL_GREETERAPPEARANCE_H_
