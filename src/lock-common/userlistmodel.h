/*
 * Copyright (C) 2026 CharOfString <root@charofstring.cc>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef SRC_LOCK_COMMON_USERLISTMODEL_H_
#define SRC_LOCK_COMMON_USERLISTMODEL_H_

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QMetaObject>
#include <memory>

class SessionBaseModel;
class User;

namespace gxde {
namespace lock {

// QML-facing model for the lock screen's user carousel. It mirrors
// SessionBaseModel's user list and keeps per-user roles (avatar, lockout,
// login state) live without resetting the model on every property change.
class UserListModel : public QAbstractListModel {
  Q_OBJECT

 public:
  enum Role {
    UserNameRole = Qt::UserRole + 1,
    DisplayNameRole,
    FullNameRole,
    AvatarRole,
    UidRole,
    LoggedInRole,
    LockedRole,
    LockMinutesRole,
    IsCurrentRole,
    UserTypeRole,
  };

  explicit UserListModel(SessionBaseModel* model, QObject* parent = nullptr);
  ~UserListModel() override;

  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  int indexOfUid(uint uid) const;
  int currentUserIndex() const;
  std::shared_ptr<User> userAt(int index) const;

  void refresh();

 private slots:
  void onUserListChanged(const QList<std::shared_ptr<User>>& users);
  void onCurrentUserChanged(const std::shared_ptr<User>& user);

 private:
  void disconnectUsers();
  void connectUser(const std::shared_ptr<User>& user);
  void notifyUserChanged(User* user);

  SessionBaseModel* model_;
  QList<std::shared_ptr<User>> users_;
  QHash<User*, QList<QMetaObject::Connection>> user_connections_;
};

}  // namespace lock
}  // namespace gxde

#endif  // SRC_LOCK_COMMON_USERLISTMODEL_H_
