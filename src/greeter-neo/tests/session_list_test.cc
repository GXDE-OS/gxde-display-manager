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

#include "greeter-neo/src/session_list.h"

#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QObject>
#include <QSet>
#include <QString>
#include <QTemporaryDir>
#include <QTest>
#include <QVariantMap>

namespace gxde::greeter {
namespace {

class SessionListTest : public QObject {
  Q_OBJECT

 private slots:
  void ReadInstalledSessions();
};

void SessionListTest::ReadInstalledSessions() {
  QTemporaryDir directory;
  QVERIFY(directory.isValid());
  const QDir root(directory.path());
  QVERIFY(root.mkpath("xsessions"));
  QVERIFY(root.mkpath("wayland-sessions"));
  const auto write = [&](const QString& path, const QByteArray& content) {
    QFile file(root.filePath(path));
    return file.open(QIODevice::WriteOnly) &&
           file.write(content) == content.size();
  };
  QVERIFY(write("xsessions/gxde.desktop",
                "[Desktop Entry]\nName=GXDE, Desktop\nExec=/bin/true\n"));
  QVERIFY(write("wayland-sessions/gxde.desktop",
                "[Desktop Entry]\nName=GXDE, Desktop\nExec=/bin/true\n"));
  QVERIFY(write("xsessions/hidden.desktop",
                "[Desktop Entry]\nName=Hidden\nExec=/bin/true\nHidden=true\n"));
  QVERIFY(
      write("xsessions/nodisplay.desktop",
            "[Desktop Entry]\nName=Hidden\nExec=/bin/true\nNoDisplay=true\n"));
  QVERIFY(write("xsessions/missing.desktop",
                "[Desktop Entry]\nName=Missing\nExec=missing\n"
                "TryExec=/nonexistent/gxde-session-test\n"));
  QVERIFY(
      write("xsessions/invalid.desktop", "[Desktop Entry]\nName=Invalid\n"));
  const auto sessions = ReadSessions({directory.path(), directory.path()});
  QCOMPARE(sessions.size(), 2);
  QSet<QString> ids;
  for (const auto& session : sessions) {
    const auto entry = session.toMap();
    QCOMPARE(entry.value("name").toString(), QString("GXDE, Desktop"));
    QCOMPARE(entry.value("file").toString(), QString("gxde.desktop"));
    ids.insert(entry.value("id").toString());
  }
  QCOMPARE(ids, QSet<QString>({"x11/gxde.desktop", "wayland/gxde.desktop"}));
  QVERIFY(ReadSessions({directory.filePath("missing")}).isEmpty());
}

}
}

QTEST_GUILESS_MAIN(gxde::greeter::SessionListTest)
#include "session_list_test.moc"
