// SPDX-License-Identifier: GPL-3.0-or-later

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusError>
#include <QDebug>

#include "SettingsAgent.h"

int main(int argc, char* argv[]) {
  QCoreApplication app(argc, argv);
  QCoreApplication::setApplicationName(
    QStringLiteral("gxdm-settings-agent"));

  SettingsAgent agent;
  new SettingsAgentAdaptor(&agent);
  QObject::connect(&agent, &SettingsAgent::idle, &app,
    &QCoreApplication::quit);

  QDBusConnection bus = QDBusConnection::sessionBus();
  if (!bus.registerObject(QStringLiteral("/top/gxde/DisplayManager"),
        &agent)) {
    qCritical() << "Cannot register /top/gxde/DisplayManager:"
      << bus.lastError().message();
    return 1;
  }
  // Register the name last: D-Bus activation hands queued calls over as soon
  // as the name is owned, so the object has to be there already.
  if (!bus.registerService(QStringLiteral("top.gxde.DisplayManager"))) {
    qCritical() << "Cannot own top.gxde.DisplayManager:"
      << bus.lastError().message();
    return 1;
  }

  return app.exec();
}
