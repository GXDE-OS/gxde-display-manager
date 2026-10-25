/*
 * Copyright (C) CharOfString <root@charofstring.cc>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <pwd.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHash>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QStringList>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>
#include <algorithm>

#include "common/GreeterAppearance.h"
#include "common/GreeterCursor.h"
#include "common/GreeterScale.h"
#include "greeter-neo/src/greeter_backend.h"
#include "greeter-neo/src/screen_manager.h"
#include "greeter-neo/src/session_list.h"
#include "greeter-neo/src/utils/macros.h"

namespace {

using ReleaseData = QHash<QString, QString>;

ReleaseData readReleaseData(const QString& path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return {};
  }

  ReleaseData data;
  QString section;

  while (!file.atEnd()) {
    QString line = QString::fromUtf8(file.readLine()).trimmed();
    if (line.isEmpty() || line.startsWith('#')) {
      continue;
    }

    if (line.startsWith('[') && line.endsWith(']')) {
      section = line.mid(1, line.size() - 2).trimmed();
      continue;
    }

    const qsizetype separator = line.indexOf('=');
    if (separator <= 0) {
      continue;
    }

    const QString key = line.left(separator).trimmed();
    QString value = line.mid(separator + 1).trimmed();

    if (value.size() >= 2 &&
        ((value.front() == '"' && value.back() == '"') ||
         (value.front() == '\'' && value.back() == '\''))) {
      value = value.mid(1, value.size() - 2);
    }

    data.insert(section.isEmpty() ? key : section + '/' + key, value);
  }

  return data;
}

struct GxdeReleaseInfo {
  bool isGxde = false;
  QString label;
};

QString localizedGxdeCodename(const QString& codename) {
  if (codename.compare(GXDE15_STABLE_CODENAME, Qt::CaseInsensitive) == 0) {
    return QString::fromUtf8(GXDE15_STABLE_CODENAME_CN);
  }

  if (codename.compare(GXDE25_STABLE_CODENAME, Qt::CaseInsensitive) == 0) {
    return QString::fromUtf8(GXDE25_STABLE_CODENAME_CN);
  }

  if (codename.compare(GXDE25_UNSTABLE_CODENAME, Qt::CaseInsensitive) == 0) {
    return QString::fromUtf8(GXDE25_UNSTABLE_CODENAME_CN);
  }

  if (codename.compare(GXDE25_LOONGARCH_UNSTABLE_CODENAME,
                       Qt::CaseInsensitive) == 0) {
    return QString::fromUtf8(GXDE25_LOONGARCH_UNSTABLE_CODENAME_CN);
  }

  return codename;
}

GxdeReleaseInfo detectGxdeRelease() {
  const ReleaseData osRelease = readReleaseData("/etc/os-release");
  GxdeReleaseInfo info;
  info.isGxde = osRelease.value("ID").compare("GXDE", Qt::CaseInsensitive) == 0;

  if (!info.isGxde) {
    return info;
  }

  const ReleaseData desktopVersion =
      readReleaseData("/usr/lib/deepin/desktop-version");
  QString version = desktopVersion.value("Release/Version");
  if (version.isEmpty()) {
    version = osRelease.value("VERSION_ID");
  }

  const QString productType = desktopVersion.value("Release/Type[zh_CN]");
  const QString milestone =
      localizedGxdeCodename(desktopVersion.value("Addition/Milestone"));
  QStringList labelParts;

  for (const QString& part : {version, productType}) {
    if (!part.isEmpty()) {
      labelParts.append(part);
    }
  }

  info.label = labelParts.join(' ');
  if (!milestone.isEmpty()) {
    info.label += QString::fromUtf8(GXDE_QUOTE_BEGIN_CN) + milestone +
                  QString::fromUtf8(GXDE_QUOTE_END_CN);
  }
  return info;
}

struct UserIdRange {
  quint64 minimum = 1000;
  quint64 maximum = 60000;
};

UserIdRange detectUserIdRange() {
  QFile file("/etc/login.defs");
  UserIdRange range;
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return range;
  }

  while (!file.atEnd()) {
    const QString line = QString::fromUtf8(file.readLine()).trimmed();
    if (line.isEmpty() || line.startsWith('#')) {
      continue;
    }

    const QStringList fields = line.simplified().split(' ');
    if (fields.size() < 2) {
      continue;
    }

    bool ok = false;
    const quint64 value = fields.at(1).toULongLong(&ok);
    if (!ok) {
      continue;
    }

    if (fields.first() == "UID_MIN") {
      range.minimum = value;
    } else if (fields.first() == "UID_MAX") {
      range.maximum = value;
    }
  }

  return range;
}

bool isLoginShell(const QString& shell) {
  return !shell.isEmpty() && !shell.endsWith("/nologin") &&
         !shell.endsWith("/false");
}

ReleaseData userAccountData(const QString& username) {
  ReleaseData data =
      readReleaseData("/var/lib/AccountsService/users/" + username);
  const ReleaseData deepinData =
      readReleaseData("/var/lib/AccountsService/deepin/users/" + username);
  for (auto it = deepinData.cbegin(); it != deepinData.cend(); ++it) {
    data.insert(it.key(), it.value());
  }
  return data;
}

QString localFilePath(const QString& reference) {
  if (!reference.startsWith("file:", Qt::CaseInsensitive)) {
    return reference;
  }
  return QUrl(reference).toLocalFile();
}

QString readableAvatarPath(const QString& reference) {
  const QFileInfo avatar(localFilePath(reference));
  if (!avatar.isFile() || !avatar.isReadable()) {
    return {};
  }

  const QFileInfo largeAvatar(
      avatar.dir().filePath("bigger/" + avatar.fileName()));
  if (largeAvatar.isFile() && largeAvatar.isReadable()) {
    return largeAvatar.absoluteFilePath();
  }
  return avatar.absoluteFilePath();
}

QString userAvatar(const QString& username, const QString& homePath,
                   const ReleaseData& accountData) {
  const QStringList candidates = {accountData.value("User/Icon"),
                                  "/var/lib/AccountsService/icons/" + username,
                                  QDir(homePath).filePath(".face"),
                                  QDir(homePath).filePath(".face.icon"),
                                  "/var/lib/AccountsService/icons/default.png"};

  for (const QString& candidate : candidates) {
    const QString path = readableAvatarPath(candidate);
    if (!path.isEmpty()) {
      return QUrl::fromLocalFile(path).toString();
    }
  }
  return {};
}

QVariantList detectUsers() {
  const UserIdRange uidRange = detectUserIdRange();
  QVariantList users;

  setpwent();
  while (const passwd* entry = getpwent()) {
    const quint64 uid = entry->pw_uid;
    if (uid < uidRange.minimum || uid > uidRange.maximum) {
      continue;
    }

    const QString username = QString::fromLocal8Bit(entry->pw_name);
    const QString shell = QString::fromLocal8Bit(entry->pw_shell);
    const QString homePath = QString::fromLocal8Bit(entry->pw_dir);
    if (username.isEmpty() || !isLoginShell(shell)) {
      continue;
    }

    const ReleaseData accountData = userAccountData(username);
    if (accountData.value("User/SystemAccount")
            .compare("true", Qt::CaseInsensitive) == 0) {
      continue;
    }

    QString fullName = accountData.value("User/RealName");
    if (fullName.isEmpty()) {
      fullName = QString::fromLocal8Bit(entry->pw_gecos).section(',', 0, 0);
    }
    if (fullName.isEmpty()) {
      fullName = username;
    }

    QVariantMap user;
    user.insert("username", username);
    user.insert("fullName", fullName);
    user.insert("avatar", userAvatar(username, homePath, accountData));
    user.insert("uid", uid);
    users.append(user);
  }
  endpwent();

  std::sort(users.begin(), users.end(),
            [](const QVariant& left, const QVariant& right) {
              return left.toMap()
                         .value("username")
                         .toString()
                         .localeAwareCompare(
                             right.toMap().value("username").toString()) < 0;
            });

  return users;
}

// The login wallpaper, chosen like gxdm-greeter-classic does: the configured
// greeter wallpaper, else the only user's own lock wallpaper, else the system
// default; the bundled one when none of them is readable.
QUrl greeterBackground(const QVariantList& users) {
  QString path;
  if (!GxdmGreeterAppearance::hasConfiguredWallpaper() && users.size() == 1) {
    const QVariantMap user = users.first().toMap();
    path = GxdmGreeterAppearance::userLockWallpaper(
        user.value("username").toString(), user.value("uid").toUInt());
  }
  if (path.isEmpty()) {
    path = GxdmGreeterAppearance::readableImage(
        GxdmGreeterAppearance::wallpaper());
  }
  return path.isEmpty() ? QUrl("qrc:/img/example-wallpaper-deepin.jpg")
                        : QUrl::fromLocalFile(path);
}

// Whether Qt will pick the xcb platform, decided before QGuiApplication
// exists the same way gxdm-greeter-classic does.
bool runsOnX11(int argc, char* argv[]) {
  QByteArray platform = qgetenv("QT_QPA_PLATFORM");
  for (int i = 1; platform.isEmpty() && i + 1 < argc; ++i) {
    if (QByteArray(argv[i]) == "-platform") platform = argv[i + 1];
  }
  return platform.startsWith("xcb") ||
         (platform.isEmpty() && !qEnvironmentVariableIsSet("WAYLAND_DISPLAY"));
}

}

int main(int argc, char* argv[]) {
  qputenv("QT_IM_MODULE", QByteArrayLiteral("qtvirtualkeyboard"));
  qputenv("QT_VIRTUALKEYBOARD_STYLE", QByteArrayLiteral("gxde"));
  GxdmGreeterCursor::applyConfiguredTheme();
  if (runsOnX11(argc, argv)) {
    GxdmGreeterScale::applyX11ScaleFactors();
  } else {
    GxdmGreeterCursor::enforceWaylandCursorConfig();
    GxdmGreeterScale::applyThemeScale();
  }
  QGuiApplication app(argc, argv);

  const QStringList arguments = app.arguments();
  const int socket_index = arguments.indexOf(QStringLiteral("--socket"));
  const QString socket_name =
      socket_index >= 0 && socket_index + 1 < arguments.size() &&
              !arguments.contains(QStringLiteral("--test-mode"))
          ? arguments.at(socket_index + 1)
          : QString();
  const auto option_values = [&arguments](const QString& option) {
    QStringList values;
    for (int i = 1; i + 1 < arguments.size(); ++i) {
      if (arguments.at(i) == option) values.append(arguments.at(++i));
    }
    return values;
  };
  QStringList x11_directories = option_values("--x11-session-dir");
  QStringList wayland_directories = option_values("--wayland-session-dir");
  if (x11_directories.isEmpty()) {
    x11_directories = {"/usr/local/share/xsessions", "/usr/share/xsessions"};
  }
  if (wayland_directories.isEmpty()) {
    wayland_directories = {"/usr/local/share/wayland-sessions",
                           "/usr/share/wayland-sessions"};
  }
  const QVariantList sessions = gxde::greeter::ReadSessionDirectories(
      x11_directories, wayland_directories);
  gxde::greeter::GreeterBackend greeter_backend(socket_name, sessions);
  QObject::connect(&greeter_backend,
                   &gxde::greeter::GreeterBackend::LoginSucceeded, &app,
                   [&app] {
                     if (QGuiApplication::platformName() ==
                         QLatin1String("xcb")) {
                       GxdmGreeterCursor::setRootWindowWaitCursor(
                           app.devicePixelRatio());
                     }
                   });
  QQmlApplicationEngine engine;
  gxde::greeter::ScreenManager screen_manager(socket_name.isEmpty());
  const GxdeReleaseInfo gxde_release = detectGxdeRelease();
  const QStringList backgrounds = option_values("--background");
  const QString background_option = GxdmGreeterAppearance::readableImage(
      backgrounds.isEmpty() ? QString() : backgrounds.last());
  const QVariantList users = detectUsers();
  QVariantMap init_prop;
  init_prop.insert("greeterBackend", QVariant::fromValue(&greeter_backend));
  init_prop.insert("screenManager", QVariant::fromValue(&screen_manager));
  init_prop.insert("backgroundUri",
                   background_option.isEmpty()
                       ? greeterBackground(users)
                       : QUrl::fromLocalFile(background_option));
  init_prop.insert("isGxdeOs", gxde_release.isGxde);
  init_prop.insert("gxdeReleaseLabel", gxde_release.label);
  init_prop.insert("userModel", users);
  init_prop.insert("sessionModel", sessions);
  engine.setInitialProperties(init_prop);

  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
      []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

  engine.loadFromModule("Greeter", "MainWindow");
  if (!engine.rootObjects().isEmpty()) {
    screen_manager.Start(engine.rootObjects()
                             .constFirst()
                             ->property("screenWindow")
                             .value<QQmlComponent*>());
  }
  return app.exec();
}
