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

#include "greeter_settings.h"

#include <QDir>
#include <QFile>
#include <QObject>
#include <QTemporaryDir>
#include <QTest>

#include "Configuration.h"

namespace {

class TestConfiguration : public SDDM::ConfigBase, public SDDM::ConfigSection {
 public:
  explicit TestConfiguration(const QString& path)
      : ConfigBase(path), ConfigSection(this, QStringLiteral("General")) {
    load();
  }
  SDDM::ConfigEntry<bool> enabled{this, QStringLiteral("EnableNeoGreeter"),
                                  true, QString()};
};

class GreeterSettingsTest : public QObject {
  Q_OBJECT
 private slots:
  void Persistence();
  void Selection();
  void WallpaperSelection();
};

void GreeterSettingsTest::Persistence() {
  SDDM::MainConfig defaults;
  defaults.EnableNeoGreeter.setDefault();
  QVERIFY(defaults.EnableNeoGreeter.get());
  QTemporaryDir directory;
  QVERIFY(directory.isValid());
  const QString path = directory.filePath(QStringLiteral("gxdm.conf"));
  QFile file(path);
  QVERIFY(file.open(QIODevice::WriteOnly));
  file.write(
      "# Keep this\n[General]\nDisplayServer=x11\n"
      "EnableNeoGreeter=true # Keep comment\n"
      "[Theme]\nCurrent=custom\n");
  file.close();
  QVERIFY(gxde::SaveGeneralSetting(path, QStringLiteral("EnableNeoGreeter"),
                                   QStringLiteral("false")));
  TestConfiguration disabled(path);
  QVERIFY(!disabled.enabled.get());
  QVERIFY(file.open(QIODevice::ReadOnly));
  const auto content = file.readAll();
  QVERIFY(content.contains("EnableNeoGreeter=false # Keep comment"));
  QVERIFY(content.contains("DisplayServer=x11"));
  QVERIFY(content.contains("[Theme]\nCurrent=custom"));
  file.close();
  QVERIFY(gxde::SaveGeneralSetting(path, QStringLiteral("EnableNeoGreeter"),
                                   QStringLiteral("true")));
  TestConfiguration enabled(path);
  QVERIFY(enabled.enabled.get());
  const QString new_path = directory.filePath(QStringLiteral("new/gxdm.conf"));
  QVERIFY(gxde::SaveGeneralSetting(new_path, QStringLiteral("EnableNeoGreeter"),
                                   QStringLiteral("false")));
  TestConfiguration created(new_path);
  QVERIFY(!created.enabled.get());
  QVERIFY(!gxde::SaveGeneralSetting(path + QStringLiteral("/invalid"),
                                    QStringLiteral("EnableNeoGreeter"),
                                    QStringLiteral("true")));
}

void GreeterSettingsTest::Selection() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());
  const QString classic =
      directory.filePath(QStringLiteral("gxdm-greeter-classic"));
  const QString neo = directory.filePath(QStringLiteral("gxdm-greeter-neo"));
  for (const QString& path : {classic, neo}) {
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();
    QVERIFY(file.setPermissions(QFileDevice::ReadOwner |
                                QFileDevice::WriteOwner |
                                QFileDevice::ExeOwner));
  }
  QCOMPARE(gxde::GreeterExecutable(true, directory.path(), 6), neo);
  QCOMPARE(gxde::GreeterExecutable(false, directory.path(), 6), classic);
  QVERIFY(QFile::remove(neo));
  QCOMPARE(gxde::GreeterExecutable(true, directory.path(), 6), classic);
  QVERIFY(QFile::remove(classic));
  QCOMPARE(gxde::GreeterExecutable(false, directory.path(), 6),
           directory.filePath(QStringLiteral("gxdm-greeter-qt6")));
}

void GreeterSettingsTest::WallpaperSelection() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());
  const QString statePath = directory.filePath(QStringLiteral("state.conf"));

  QCOMPARE(gxde::StateConfigPath(directory.path()), statePath);
  // 未配置 [Greeter] Wallpaper：必须返回空，让 Greeter 跟随用户壁纸，
  // 而不是把内置默认图转发成 --background。
  QCOMPARE(gxde::ConfiguredGreeterWallpaper(statePath), QString());

  QFile file(statePath);
  QVERIFY(file.open(QIODevice::WriteOnly));
  file.write("[Last]\nUser=shenmo-ikun\n");
  file.close();
  QCOMPARE(gxde::ConfiguredGreeterWallpaper(statePath), QString());

  // ClearWallpaper 写入的空值同样视为未配置。
  QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
  file.write("[Greeter]\nWallpaper=\n");
  file.close();
  QCOMPARE(gxde::ConfiguredGreeterWallpaper(statePath), QString());

  // 显式配置的全局壁纸需要原样返回（去掉首尾空白）。
  QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
  file.write("[Greeter]\nWallpaper= /var/lib/gxdm/greeter-wallpaper \n");
  file.close();
  QCOMPARE(gxde::ConfiguredGreeterWallpaper(statePath),
           QStringLiteral("/var/lib/gxdm/greeter-wallpaper"));
}

}

QTEST_GUILESS_MAIN(GreeterSettingsTest)
#include "greeter_settings_test.moc"
