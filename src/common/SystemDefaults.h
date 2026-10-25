#ifndef SYSTEMDEFAULTS_H
#define SYSTEMDEFAULTS_H

// OS defaults shared by gxdm-greeter-classic, gxdm-lock-classic and
// gxdm-greeter-neo.

#include <QString>

namespace GxdmSystemDefaults {

QString operatingSystemId(const QString &osReleasePath = QString());
bool isGxdeOperatingSystem(const QString &osReleasePath = QString());
QString defaultSessionDesktopFile(const QString &osReleasePath = QString());
QString initialSessionDesktopFile(const QString &rememberedSession,
    const QString &osReleasePath = QString());

} // namespace GxdmSystemDefaults

#endif // SYSTEMDEFAULTS_H
