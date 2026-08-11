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

  const QString statePath = directory.filePath(QStringLiteral("state.conf"));
  QSettings settings(statePath, QSettings::IniFormat);
  settings.setValue(
    QStringLiteral("Greeter/CursorTheme"), QStringLiteral("gxde"));
  settings.setValue(QStringLiteral("Greeter/Wallpaper"), wallpaperPath);
  settings.sync();

  QCOMPARE(
    GxdmGreeterAppearance::cursorTheme(statePath), QStringLiteral("gxde"));
  QCOMPARE(GxdmGreeterAppearance::wallpaper(statePath), wallpaperPath);
  QCOMPARE(GxdmGreeterAppearance::lockWallpaperOverride(
    uid, directory.path()), lockOverridePath);
  QCOMPARE(GxdmGreeterAppearance::lockWallpaperOverridePath(
    uid, directory.path()), lockOverridePath);
  QVERIFY(QFile::remove(lockOverridePath));
  QCOMPARE(GxdmGreeterAppearance::lockWallpaperOverride(
    uid, directory.path()), QString());
}

void GreeterAppearanceTest::exposesDefaultWallpaperPaths() {
  QCOMPARE(GxdmGreeterAppearance::gxdeDefaultWallpaper(),
    QStringLiteral("/usr/share/backgrounds/default_background.jpg"));
  QCOMPARE(GxdmGreeterAppearance::ddeLockDefaultWallpaper(),
    QStringLiteral(":/theme/background/default_background.jpg"));
}

QTEST_MAIN(GreeterAppearanceTest)
#include "GreeterAppearanceTest.moc"
