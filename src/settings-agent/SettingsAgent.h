// SPDX-License-Identifier: GPL-3.0-or-later
//
// Session-bus front end for the greeter settings of the GXDM system daemon.
//
// The in-session lock screen moved to gxde-lock, which owns its own bus
// names. This agent keeps the session-bus top.gxde.DisplayManager name that
// gxde-control-center and gxde-lock call: it forwards each request to
// top.gxde.DisplayManager.System on the system bus, passing wallpapers as file
// descriptors and the caller's uid for per-user overrides. It is D-Bus
// activated and exits after a short idle period.

#ifndef GXDE_DISPLAY_MANAGER_SETTINGS_AGENT_SETTINGSAGENT_H_
#define GXDE_DISPLAY_MANAGER_SETTINGS_AGENT_SETTINGSAGENT_H_

#include <QDBusAbstractAdaptor>
#include <QObject>
#include <QTimer>

class SettingsAgent : public QObject {
  Q_OBJECT

 public:
  explicit SettingsAgent(QObject* parent = nullptr);

  // Restarts the idle timer; called on every D-Bus request.
  void touch();

 signals:
  void idle();

 private:
  QTimer m_idleTimer;
};

class SettingsAgentAdaptor : public QDBusAbstractAdaptor {
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "top.gxde.DisplayManager")
  Q_PROPERTY(QString GreeterDisplayServer READ GreeterDisplayServer)
  Q_PROPERTY(QString LockWallpaperOverride READ LockWallpaperOverride)

 public:
  explicit SettingsAgentAdaptor(SettingsAgent* agent);

 public slots:
  bool SetCursor(const QString& theme);
  bool SetWallpaperGXDEDefault();
  bool SetWallpaperDDELockDefault();
  bool SetWallpaper(const QString& wallpaper);
  bool ClearWallpaper();
  // The calling user's lock wallpaper, shown by the greeter for that user.
  // gxde-lock pushes it here whenever the user changes it.
  bool SetLockWallpaperOverride(const QString& wallpaper);
  bool ClearLockWallpaperOverride();
  QString LockWallpaperOverride() const;
  bool SetGreeterDisplayServer(const QString& displayServer);
  QString GreeterDisplayServer() const;
  bool SetNeoGreeterEnabled(bool enabled);
  bool NeoGreeterEnabled() const;

 private:
  SettingsAgent* m_agent = nullptr;
};

#endif  // GXDE_DISPLAY_MANAGER_SETTINGS_AGENT_SETTINGSAGENT_H_
