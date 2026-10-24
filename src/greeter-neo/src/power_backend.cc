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
#include <QLocalSocket>
#include <QString>
#include <QStringList>
#include <QtGlobal>

#include "common/Messages.h"

namespace gxde::greeter {
namespace {

struct PowerAction {
  const char* name;
  SDDM::Capability capability;
  SDDM::GreeterMessages message;
};

constexpr PowerAction kActions[] = {
    {"reboot", SDDM::Capability::Reboot, SDDM::GreeterMessages::Reboot},
    {"shutdown", SDDM::Capability::PowerOff, SDDM::GreeterMessages::PowerOff},
    {"suspend", SDDM::Capability::Suspend, SDDM::GreeterMessages::Suspend}};

}

PowerBackend::PowerBackend(const QString& socket_name, QObject* parent)
    : QObject(parent), input_(&socket_) {
  connect(&socket_, &QLocalSocket::connected, this, [this] {
    SendMessage(static_cast<quint32>(SDDM::GreeterMessages::Connect));
  });
  connect(&socket_, &QLocalSocket::readyRead, this,
          &PowerBackend::ReadMessages);
  connect(&socket_, &QLocalSocket::disconnected, this,
          [this] { SetCapabilities(0); });
  connect(&socket_, &QLocalSocket::errorOccurred, this, [this] {
    SetCapabilities(0);
    emit RequestFailed(tr("电源服务连接失败：%1").arg(socket_.errorString()));
  });
  if (!socket_name.isEmpty()) {
    socket_.connectToServer(socket_name);
  }
}

QStringList PowerBackend::AvailableActions() const {
  QStringList available;
  for (const auto& action : kActions) {
    if (capabilities_ & action.capability) {
      available.append(QString::fromLatin1(action.name));
    }
  }
  return available;
}

void PowerBackend::SetCapabilities(quint32 capabilities) {
  if (capabilities_ == capabilities) {
    return;
  }
  capabilities_ = capabilities;
  emit AvailableActionsChanged();
}

bool PowerBackend::SendMessage(quint32 message) {
  QByteArray data;
  QDataStream output(&data, QIODevice::WriteOnly);
  output << message;
  if (socket_.state() != QLocalSocket::ConnectedState ||
      socket_.write(data) != data.size()) {
    emit RequestFailed(tr("无法发送电源请求，请稍后重试。"));
    return false;
  }
  socket_.flush();
  return true;
}

void PowerBackend::Request(const QString& name) {
  for (const auto& action : kActions) {
    if (name != QLatin1String(action.name)) {
      continue;
    }
    if (!(capabilities_ & action.capability)) {
      emit RequestFailed(tr("当前无法执行此电源操作。"));
      return;
    }
    SendMessage(static_cast<quint32>(action.message));
    return;
  }
  emit RequestFailed(tr("未知的电源操作。"));
}

void PowerBackend::ReadMessages() {
  while (socket_.bytesAvailable() > 0) {
    input_.startTransaction();
    quint32 message = 0;
    quint32 capabilities = 0;
    QString text;
    input_ >> message;
    if (input_.status() != QDataStream::Ok) {
      input_.rollbackTransaction();
      return;
    }
    switch (static_cast<SDDM::DaemonMessages>(message)) {
      case SDDM::DaemonMessages::Capabilities:
        input_ >> capabilities;
        break;
      case SDDM::DaemonMessages::HostName:
      case SDDM::DaemonMessages::InformationMessage:
      case SDDM::DaemonMessages::LastSession:
        input_ >> text;
        break;
      case SDDM::DaemonMessages::LoginSucceeded:
      case SDDM::DaemonMessages::LoginFailed:
        break;
      default:
        input_.abortTransaction();
        socket_.abort();
        SetCapabilities(0);
        emit RequestFailed(tr("电源服务返回了无法识别的消息。"));
        return;
    }
    if (!input_.commitTransaction()) {
      return;
    }
    if (static_cast<SDDM::DaemonMessages>(message) ==
        SDDM::DaemonMessages::Capabilities) {
      SetCapabilities(capabilities);
    }
  }
}

}
