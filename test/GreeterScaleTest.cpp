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

#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include "GreeterScale.h"

using Environment = QPair<QByteArray, QByteArray>;

class GreeterScaleTest : public QObject {
  Q_OBJECT

 private slots:
  void readsThemeScale_data();
  void readsThemeScale();
  void ignoresMissingFile();
};

void GreeterScaleTest::readsThemeScale_data() {
  QTest::addColumn<QByteArray>("contents");
  QTest::addColumn<QByteArray>("variable");
  QTest::addColumn<QByteArray>("value");

  QTest::newRow("single factor")
    << QByteArray("[Theme]\nIconThemeName=deepin\nScreenScaleFactors=1.50\n"
                  "ScaleLogicalDpi=96,96\n")
    << QByteArray("QT_SCALE_FACTOR") << QByteArray("1.50");
  QTest::newRow("per screen")
    << QByteArray("[Theme]\nScreenScaleFactors=eDP-1=2;HDMI-1=1\n")
    << QByteArray("QT_SCREEN_SCALE_FACTORS")
    << QByteArray("eDP-1=2;HDMI-1=1");
  QTest::newRow("scale factor fallback")
    << QByteArray("[Theme]\nScaleFactor=2\n")
    << QByteArray("QT_SCALE_FACTOR") << QByteArray("2");
  QTest::newRow("other group")
    << QByteArray("[Other]\nScreenScaleFactors=2\n[Theme]\nFont=Sans\n")
    << QByteArray() << QByteArray();
  QTest::newRow("invalid factor")
    << QByteArray("[Theme]\nScreenScaleFactors=big\nScaleFactor=0\n")
    << QByteArray() << QByteArray();
}

void GreeterScaleTest::readsThemeScale() {
  QFETCH(QByteArray, contents);
  QFETCH(QByteArray, variable);
  QFETCH(QByteArray, value);

  QTemporaryDir directory;
  QVERIFY(directory.isValid());
  const QString path = directory.filePath(QStringLiteral("qt-theme.ini"));
  QFile file(path);
  QVERIFY(file.open(QIODevice::WriteOnly));
  QCOMPARE(file.write(contents), contents.size());
  file.close();

  QCOMPARE(GxdmGreeterScale::themeScaleEnvironment(path),
           Environment(variable, value));
}

void GreeterScaleTest::ignoresMissingFile() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());
  QCOMPARE(GxdmGreeterScale::themeScaleEnvironment(
             directory.filePath(QStringLiteral("missing.ini"))),
           Environment());
}

QTEST_GUILESS_MAIN(GreeterScaleTest)
#include "GreeterScaleTest.moc"
