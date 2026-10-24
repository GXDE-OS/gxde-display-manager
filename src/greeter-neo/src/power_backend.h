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

#ifndef GXDE_DISPLAY_MANAGER_GREETER_NEO_SRC_POWER_BACKEND_H_
#define GXDE_DISPLAY_MANAGER_GREETER_NEO_SRC_POWER_BACKEND_H_

#include <QDataStream>
#include <QLocalSocket>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QtGlobal>

namespace gxde::greeter {

class PowerBackend : public QObject {
  Q_OBJECT
  Q_PROPERTY(QStringList availableActions READ AvailableActions NOTIFY
                 AvailableActionsChanged)

 public:
  explicit PowerBackend(const QString& socket_name, QObject* parent = nullptr);
  QStringList AvailableActions() const;
  Q_INVOKABLE void Request(const QString& action);

 signals:
  void AvailableActionsChanged();
  void RequestFailed(const QString& message);

 private:
  void ReadMessages();
  void SetCapabilities(quint32 capabilities);
  bool SendMessage(quint32 message);

  QLocalSocket socket_;
  QDataStream input_;
  quint32 capabilities_ = 0;
};

}

#endif
