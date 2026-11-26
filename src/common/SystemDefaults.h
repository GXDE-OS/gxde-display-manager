#ifndef SYSTEMDEFAULTS_H
#define SYSTEMDEFAULTS_H

// OS defaults shared by gxdm-greeter-classic and gxdm-greeter-neo.

#include <QString>

namespace GxdmSystemDefaults {

QString operatingSystemId(const QString &osReleasePath = QString());
bool isGxdeOperatingSystem(const QString &osReleasePath = QString());
QString defaultSessionDesktopFile(const QString &osReleasePath = QString());
QString initialSessionDesktopFile(const QString &rememberedSession,
    const QString &osReleasePath = QString());

// True for the session files that used to be GXDE's default session before
// flakewm (the X11 deepin.desktop and the gxde-wlcom Wayland session).
bool isLegacyDefaultSession(const QString &sessionDesktopFile);

} // namespace GxdmSystemDefaults

#endif // SYSTEMDEFAULTS_H
