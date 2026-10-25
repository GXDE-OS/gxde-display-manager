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

#include "greeter-neo/src/greeter_backend.h"

#include <QByteArray>
#include <QDataStream>
#include <QDebug>
#include <QFileInfo>
#include <QIODevice>
#include <QLocalSocket>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QtGlobal>

#include "common/Messages.h"
#include "common/Session.h"
#include "common/SystemDefaults.h"

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

GreeterBackend::GreeterBackend(const QString& socket_name,
                               const QVariantList& sessions, QObject* parent)
    : QObject(parent), input_(&socket_), sessions_(sessions) {
  connect(&socket_, &QLocalSocket::connected, this, [this] {
    emit ConnectedChanged();
    SendMessage(static_cast<quint32>(SDDM::GreeterMessages::Connect));
  });
  connect(&socket_, &QLocalSocket::readyRead, this,
          &GreeterBackend::ReadMessages);
  connect(&socket_, &QLocalSocket::disconnected, this,
          &GreeterBackend::HandleDisconnect);
  connect(&socket_, &QLocalSocket::errorOccurred, this, [this] {
    HandleDisconnect();
    if (login_succeeded_) return;
    emit RequestFailed(tr("电源服务连接失败：%1").arg(socket_.errorString()));
  });
  if (!socket_name.isEmpty()) {
    socket_.connectToServer(socket_name);
  }
}

bool GreeterBackend::IsConnected() const {
  return socket_.state() == QLocalSocket::ConnectedState;
}

bool GreeterBackend::IsAuthenticating() const { return authenticating_; }

QString GreeterBackend::InitialSession(const QString& remembered) const {
  const QString desktop_file =
      GxdmSystemDefaults::initialSessionDesktopFile(remembered);
  const bool using_system_default =
      remembered.isEmpty() && !desktop_file.isEmpty();
  if (desktop_file.isEmpty()) return {};

  const QString name = QFileInfo(desktop_file).completeBaseName();
  for (const QVariant& candidate : sessions_) {
    const QVariantMap session = candidate.toMap();
    const QString file = session.value("file").toString();
    if (QFileInfo(file).completeBaseName().compare(name, Qt::CaseInsensitive) !=
        0) {
      continue;
    }
    const QString id = session.value("id").toString();
    if (using_system_default) {
      qDebug() << "(Frontend) No remembered session; Defaulting to"
               << desktop_file << id;
    } else {
      qDebug() << "(Frontend) Restoring last session:" << desktop_file << id;
    }
    return id;
  }

  if (using_system_default) {
    qWarning() << "(Frontend) GXDE default session is unavailable:"
               << desktop_file;
  } else {
    qWarning() << "(Frontend) Remembered session is no longer available:"
               << desktop_file;
  }
  return {};
}

QString GreeterBackend::LastSession() const { return last_session_; }

void GreeterBackend::SetAuthenticating(bool authenticating) {
  if (authenticating_ == authenticating) return;
  authenticating_ = authenticating;
  emit AuthenticatingChanged();
}

void GreeterBackend::HandleDisconnect() {
  SetCapabilities(0);
  emit ConnectedChanged();
  if (authenticating_ && !login_succeeded_) {
    SetAuthenticating(false);
    emit LoginFailed(tr("登录服务连接已断开，请稍后重试。"));
  }
}

void GreeterBackend::Login(const QString& username, const QString& password,
                           const QString& session_id) {
  if (authenticating_ || login_succeeded_) return;
  if (!IsConnected()) {
    emit LoginFailed(tr("登录服务不可用。"));
    return;
  }
  if (username.trimmed().isEmpty()) {
    emit LoginFailed(tr("请输入用户名。"));
    return;
  }
  QVariantMap session;
  for (const auto& candidate : sessions_) {
    if (candidate.toMap().value("id").toString() == session_id) {
      session = candidate.toMap();
      break;
    }
  }
  const QString type = session.value("type").toString();
  const QString file = session.value("file").toString();
  if ((type != "x11" && type != "wayland") || file.isEmpty()) {
    emit LoginFailed(tr("请选择可用的桌面会话。"));
    return;
  }
  QByteArray data;
  QDataStream output(&data, QIODevice::WriteOnly);
  output << static_cast<quint32>(SDDM::GreeterMessages::Login) << username
         << password
         << static_cast<quint32>(type == "wayland"
                                     ? SDDM::Session::WaylandSession
                                     : SDDM::Session::X11Session)
         << file;
  SetAuthenticating(true);
  if (socket_.write(data) != data.size()) {
    SetAuthenticating(false);
    emit LoginFailed(tr("无法发送登录请求。"));
  }
  data.fill('\0');
  socket_.flush();
}

QStringList GreeterBackend::AvailableActions() const {
  QStringList available;
  for (const auto& action : kActions) {
    if (capabilities_ & action.capability) {
      available.append(QString::fromLatin1(action.name));
    }
  }
  return available;
}

void GreeterBackend::SetCapabilities(quint32 capabilities) {
  if (capabilities_ == capabilities) {
    return;
  }
  capabilities_ = capabilities;
  emit AvailableActionsChanged();
}

bool GreeterBackend::SendMessage(quint32 message) {
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

void GreeterBackend::Request(const QString& name) {
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

void GreeterBackend::ReadMessages() {
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
    } else if (static_cast<SDDM::DaemonMessages>(message) ==
               SDDM::DaemonMessages::LastSession) {
      last_session_ = InitialSession(text);
      emit LastSessionChanged();
    } else if (static_cast<SDDM::DaemonMessages>(message) ==
               SDDM::DaemonMessages::LoginFailed) {
      SetAuthenticating(false);
      emit LoginFailed(tr("鉴权失败"));
    } else if (static_cast<SDDM::DaemonMessages>(message) ==
               SDDM::DaemonMessages::LoginSucceeded) {
      login_succeeded_ = true;
      SetAuthenticating(false);
      emit LoginSucceeded();
    } else if (static_cast<SDDM::DaemonMessages>(message) ==
               SDDM::DaemonMessages::InformationMessage) {
      emit InformationMessage(text);
    }
  }
}

}
