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

#include "greeter-neo/src/power_backend.h"

#include <QByteArray>
#include <QDataStream>
#include <QIODevice>
#include <QLocalServer>
#include <QLocalSocket>
#include <QObject>
#include <QSignalSpy>
#include <QString>
#include <QStringList>
#include <QTest>
#include <QUuid>
#include <QtGlobal>

#include "common/Messages.h"

namespace gxde::greeter {
namespace {

class PowerBackendTest : public QObject {
  Q_OBJECT

 private slots:
  void Standalone() {
    PowerBackend backend{QString()};
    QSignalSpy errors(&backend, &PowerBackend::RequestFailed);
    QVERIFY(backend.AvailableActions().isEmpty());
    backend.Request("shutdown");
    QCOMPARE(errors.size(), 1);
  }

  void Protocol() {
    QLocalServer server;
    QVERIFY(server.listen("gxde-power-test-" + QUuid::createUuid().toString()));
    PowerBackend backend(server.fullServerName());
    QSignalSpy errors(&backend, &PowerBackend::RequestFailed);
    QTRY_VERIFY(server.hasPendingConnections());
    auto* peer = server.nextPendingConnection();
    QTRY_COMPARE(peer->bytesAvailable(), 4);
    QDataStream input(peer);
    quint32 message;
    input >> message;
    QCOMPARE(message, static_cast<quint32>(SDDM::GreeterMessages::Connect));
    QVERIFY(backend.AvailableActions().isEmpty());

    QByteArray data;
    QDataStream output(&data, QIODevice::WriteOnly);
    output << static_cast<quint32>(SDDM::DaemonMessages::HostName)
           << QString("test-host")
           << static_cast<quint32>(SDDM::DaemonMessages::LastSession)
           << QString("test.desktop")
           << static_cast<quint32>(SDDM::DaemonMessages::Capabilities)
           << static_cast<quint32>(SDDM::Capability::PowerOff |
                                   SDDM::Capability::Reboot);
    for (int i = 0; i < data.size(); ++i) {
      QCOMPARE(peer->write(data.mid(i, 1)), 1);
      peer->flush();
      QTest::qWait(1);
      if (i + 1 < data.size()) {
        QVERIFY(backend.AvailableActions().isEmpty());
      }
    }
    QTRY_COMPARE(backend.AvailableActions(),
                 QStringList({"reboot", "shutdown"}));
    backend.Request("suspend");
    backend.Request("invalid");
    QCOMPARE(errors.size(), 2);
    QCOMPARE(peer->bytesAvailable(), 0);
    backend.Request("reboot");
    backend.Request("shutdown");
    QTRY_COMPARE(peer->bytesAvailable(), 8);
    input >> message;
    QCOMPARE(message, static_cast<quint32>(SDDM::GreeterMessages::Reboot));
    input >> message;
    QCOMPARE(message, static_cast<quint32>(SDDM::GreeterMessages::PowerOff));

    data.clear();
    QDataStream update(&data, QIODevice::WriteOnly);
    update << static_cast<quint32>(SDDM::DaemonMessages::Capabilities)
           << static_cast<quint32>(SDDM::Capability::Suspend);
    peer->write(data);
    peer->flush();
    QTRY_COMPARE(backend.AvailableActions(), QStringList({"suspend"}));
    backend.Request("suspend");
    QTRY_COMPARE(peer->bytesAvailable(), 4);
    input >> message;
    QCOMPARE(message, static_cast<quint32>(SDDM::GreeterMessages::Suspend));

    peer->disconnectFromServer();
    QTRY_VERIFY(backend.AvailableActions().isEmpty());
    QTRY_COMPARE(errors.size(), 3);
    backend.Request("reboot");
    QCOMPARE(errors.size(), 4);
  }

  void UnknownMessage() {
    QLocalServer server;
    QVERIFY(server.listen("gxde-power-test-" + QUuid::createUuid().toString()));
    PowerBackend backend(server.fullServerName());
    QSignalSpy errors(&backend, &PowerBackend::RequestFailed);
    QTRY_VERIFY(server.hasPendingConnections());
    auto* peer = server.nextPendingConnection();
    QByteArray data;
    QDataStream output(&data, QIODevice::WriteOnly);
    output << static_cast<quint32>(SDDM::DaemonMessages::Capabilities)
           << static_cast<quint32>(7) << static_cast<quint32>(999);
    peer->write(data);
    peer->flush();
    QTRY_COMPARE(errors.size(), 1);
    QVERIFY(backend.AvailableActions().isEmpty());
  }
};

}
}

QTEST_GUILESS_MAIN(gxde::greeter::PowerBackendTest)
#include "power_backend_test.moc"
