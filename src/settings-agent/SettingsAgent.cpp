// SPDX-License-Identifier: GPL-3.0-or-later

#include "SettingsAgent.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusUnixFileDescriptor>
#include <QDebug>
#include <QFile>
#include <QUrl>

#include <unistd.h>

namespace {

const QString kSystemService = QStringLiteral("top.gxde.DisplayManager");
const QString kSystemPath = QStringLiteral("/top/gxde/DisplayManager");
const QString kSystemInterface =
  QStringLiteral("top.gxde.DisplayManager.System");

constexpr int kIdleTimeoutMs = 60 * 1000;

template <typename T>
T callSystem(const QString& method, const QVariantList& arguments = {}) {
  QDBusInterface system(kSystemService, kSystemPath, kSystemInterface,
    QDBusConnection::systemBus());
  const QDBusReply<T> reply(
    system.callWithArgumentList(QDBus::Block, method, arguments));
  if (!reply.isValid()) {
    qWarning() << method << "failed:" << reply.error().message();
    return T();
  }
  return reply.value();
}

QString localPath(const QString& pathOrUrl) {
  const QUrl url(pathOrUrl);
  return url.isLocalFile() ? url.toLocalFile() : pathOrUrl;
}

// The daemon runs as root, so it never opens a caller-supplied path: it gets
// a descriptor this agent opened with the caller's own permissions, and
// validates and copies the image itself.
bool sendWallpaper(const QString& method, const QString& wallpaper,
    QVariantList arguments = {}) {
  QFile file(localPath(wallpaper));
  if (!file.open(QIODevice::ReadOnly)) {
    qWarning() << method << "cannot open" << file.fileName();
    return false;
  }

  const QDBusUnixFileDescriptor descriptor(file.handle());
  if (!descriptor.isValid()) return false;

  arguments << QVariant::fromValue(descriptor);
  return callSystem<bool>(method, arguments);
}

QVariant currentUid() {
  return QVariant::fromValue(static_cast<uint>(getuid()));
}

}  // namespace

SettingsAgent::SettingsAgent(QObject* parent) : QObject(parent) {
  m_idleTimer.setSingleShot(true);
  m_idleTimer.setInterval(kIdleTimeoutMs);
  connect(&m_idleTimer, &QTimer::timeout, this, &SettingsAgent::idle);
  m_idleTimer.start();
}

void SettingsAgent::touch() { m_idleTimer.start(); }

SettingsAgentAdaptor::SettingsAgentAdaptor(SettingsAgent* agent)
    : QDBusAbstractAdaptor(agent), m_agent(agent) {}

bool SettingsAgentAdaptor::SetCursor(const QString& theme) {
  m_agent->touch();
  return callSystem<bool>(QStringLiteral("SetCursor"), {theme});
}

bool SettingsAgentAdaptor::SetWallpaperGXDEDefault() {
  m_agent->touch();
  return callSystem<bool>(QStringLiteral("SetWallpaperGXDEDefault"));
}

bool SettingsAgentAdaptor::SetWallpaperDDELockDefault() {
  m_agent->touch();
  return callSystem<bool>(QStringLiteral("SetWallpaperDDELockDefault"));
}

bool SettingsAgentAdaptor::SetWallpaper(const QString& wallpaper) {
  m_agent->touch();
  return sendWallpaper(QStringLiteral("SetWallpaper"), wallpaper);
}

bool SettingsAgentAdaptor::ClearWallpaper() {
  m_agent->touch();
  return callSystem<bool>(QStringLiteral("ClearWallpaper"));
}

bool SettingsAgentAdaptor::SetLockWallpaperOverride(const QString& wallpaper) {
  m_agent->touch();
  return sendWallpaper(QStringLiteral("SetLockWallpaperOverride"), wallpaper,
    {currentUid()});
}

bool SettingsAgentAdaptor::ClearLockWallpaperOverride() {
  m_agent->touch();
  return callSystem<bool>(QStringLiteral("ClearLockWallpaperOverride"),
    {currentUid()});
}

QString SettingsAgentAdaptor::LockWallpaperOverride() const {
  m_agent->touch();
  return callSystem<QString>(QStringLiteral("LockWallpaperOverride"),
    {currentUid()});
}

bool SettingsAgentAdaptor::SetGreeterDisplayServer(
    const QString& displayServer) {
  m_agent->touch();
  return callSystem<bool>(QStringLiteral("SetGreeterDisplayServer"),
    {displayServer});
}

QString SettingsAgentAdaptor::GreeterDisplayServer() const {
  m_agent->touch();
  return callSystem<QString>(QStringLiteral("GreeterDisplayServer"));
}

bool SettingsAgentAdaptor::SetNeoGreeterEnabled(bool enabled) {
  m_agent->touch();
  return callSystem<bool>(QStringLiteral("SetNeoGreeterEnabled"), {enabled});
}

bool SettingsAgentAdaptor::NeoGreeterEnabled() const {
  m_agent->touch();
  return callSystem<bool>(QStringLiteral("NeoGreeterEnabled"));
}
