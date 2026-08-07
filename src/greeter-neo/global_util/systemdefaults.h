#ifndef SYSTEMDEFAULTS_H
#define SYSTEMDEFAULTS_H

#include <QString>

namespace GxdmSystemDefaults {

QString operatingSystemId(const QString &osReleasePath = QString());
bool isGxdeOperatingSystem(const QString &osReleasePath = QString());
QString defaultSessionDesktopFile(const QString &osReleasePath = QString());
QString initialSessionDesktopFile(const QString &rememberedSession,
    const QString &osReleasePath = QString());

} // namespace GxdmSystemDefaults

#endif // SYSTEMDEFAULTS_H
