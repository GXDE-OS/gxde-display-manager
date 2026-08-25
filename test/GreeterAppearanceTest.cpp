#include <QFile>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

#include "greeterappearance.h"

class GreeterAppearanceTest : public QObject {
  Q_OBJECT

 private slots:
  void readsGlobalAppearance();
  void exposesDefaultWallpaperPaths();
  void detectsUnconfiguredWallpaper();
};

void GreeterAppearanceTest::readsGlobalAppearance() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  const QString wallpaperPath = directory.filePath(QStringLiteral("wallpaper"));
  QFile wallpaper(wallpaperPath);
  QVERIFY(wallpaper.open(QIODevice::WriteOnly));
  QCOMPARE(wallpaper.write("image"), 5);
  wallpaper.close();

  const uint uid = 1000;
  const QString lockOverridePath = directory.filePath(
    QStringLiteral("lock-wallpaper-override"));
  QFile lockOverride(lockOverridePath);
  QVERIFY(lockOverride.open(QIODevice::WriteOnly));
  QCOMPARE(lockOverride.write("image"), 5);
  lockOverride.close();

  const QString systemLockOverridePath = directory.filePath(
    QStringLiteral("lock-wallpaper-override-%1").arg(uid));
  QFile systemLockOverride(systemLockOverridePath);
  QVERIFY(systemLockOverride.open(QIODevice::WriteOnly));
  QCOMPARE(systemLockOverride.write("image"), 5);
  systemLockOverride.close();

  const QString statePath = directory.filePath(QStringLiteral("state.conf"));
  QSettings settings(statePath, QSettings::IniFormat);
  settings.setValue(
    QStringLiteral("Greeter/CursorTheme"), QStringLiteral("gxde"));
  settings.setValue(QStringLiteral("Greeter/Wallpaper"), wallpaperPath);
  settings.sync();

  QCOMPARE(
    GxdmGreeterAppearance::cursorTheme(statePath), QStringLiteral("gxde"));
  QCOMPARE(GxdmGreeterAppearance::wallpaper(statePath), wallpaperPath);
  QVERIFY(GxdmGreeterAppearance::hasConfiguredWallpaper(statePath));
  QCOMPARE(GxdmGreeterAppearance::lockWallpaperOverride(
    uid, directory.path()), lockOverridePath);
  QCOMPARE(GxdmGreeterAppearance::lockWallpaperOverridePath(
    uid, directory.path()), lockOverridePath);
  QCOMPARE(GxdmGreeterAppearance::systemLockWallpaperOverridePath(
    uid, directory.path()), systemLockOverridePath);
  QCOMPARE(GxdmGreeterAppearance::systemLockWallpaperOverride(
    uid, directory.path()), systemLockOverridePath);
  QVERIFY(QFile::remove(lockOverridePath));
  QVERIFY(QFile::remove(systemLockOverridePath));
  QCOMPARE(GxdmGreeterAppearance::lockWallpaperOverride(
    uid, directory.path()), QString());
  QCOMPARE(GxdmGreeterAppearance::systemLockWallpaperOverride(
    uid, directory.path()), QString());
}

void GreeterAppearanceTest::exposesDefaultWallpaperPaths() {
  QCOMPARE(GxdmGreeterAppearance::gxdeDefaultWallpaper(),
    QStringLiteral("/usr/share/backgrounds/default_background.jpg"));
  QCOMPARE(GxdmGreeterAppearance::ddeLockDefaultWallpaper(),
    QStringLiteral(":/theme/background/default_background.jpg"));
}

void GreeterAppearanceTest::detectsUnconfiguredWallpaper() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());

  // state.conf 存在但没有写入 [Greeter] Wallpaper
  const QString statePath = directory.filePath(QStringLiteral("state.conf"));
  QSettings settings(statePath, QSettings::IniFormat);
  settings.setValue(
    QStringLiteral("Greeter/CursorTheme"), QStringLiteral("gxde"));
  settings.sync();

  QVERIFY(!GxdmGreeterAppearance::hasConfiguredWallpaper(statePath));
  // state.conf 文件不存在时同样视为未配置
  QVERIFY(!GxdmGreeterAppearance::hasConfiguredWallpaper(
    directory.filePath(QStringLiteral("missing.conf"))));

  // ClearWallpaper 写入空值 "Wallpaper="，同样视为未配置
  settings.setValue(QStringLiteral("Greeter/Wallpaper"), QString());
  settings.sync();
  QVERIFY(!GxdmGreeterAppearance::hasConfiguredWallpaper(statePath));
}

QTEST_MAIN(GreeterAppearanceTest)
#include "GreeterAppearanceTest.moc"
