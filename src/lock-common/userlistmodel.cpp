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

#include "userlistmodel.h"

#include "sessionbasemodel.h"
#include "userinfo.h"

#include <QUrl>

namespace gxde {
namespace lock {
namespace {

QString imageUri(const QString& path) {
  if (path.isEmpty()) return QString();

  // The user model returns Qt resource paths such as ":/img/default_avatar.png".
  // QML's Image.source expects the qrc scheme.
  if (path.startsWith(QLatin1String(":/"))) {
    return QStringLiteral("qrc") + path;
  }
  if (path.startsWith(QLatin1String("qrc:/")) ||
      path.startsWith(QLatin1String("file:"))) {
    return path;
  }
  return QUrl::fromLocalFile(path).toString();
}

}  // namespace

UserListModel::UserListModel(SessionBaseModel* model, QObject* parent)
    : QAbstractListModel(parent), model_(model) {
  if (model_) {
    connect(model_, &SessionBaseModel::onUserListChanged, this,
            &UserListModel::onUserListChanged);
    connect(model_, &SessionBaseModel::currentUserChanged, this,
            &UserListModel::onCurrentUserChanged);
  }
  refresh();
}

UserListModel::~UserListModel() {
  disconnectUsers();
}

int UserListModel::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : static_cast<int>(users_.size());
}

QVariant UserListModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= users_.size()) {
    return QVariant();
  }

  const std::shared_ptr<User>& user = users_.at(index.row());
  if (!user) return QVariant();

  switch (role) {
    case UserNameRole:
      return user->name();
    case DisplayNameRole:
      return user->displayName();
    case FullNameRole:
      return user->displayName();
    case AvatarRole:
      return imageUri(user->avatarPath());
    case UidRole:
      return user->uid();
    case LoggedInRole:
      return user->isLogin();
    case LockedRole:
      return user->isLock();
    case LockMinutesRole:
      return static_cast<int>(user->lockNum());
    case IsCurrentRole:
      return model_ && model_->currentUser() == user;
    case UserTypeRole:
      return static_cast<int>(user->type());
    default:
      return QVariant();
  }
}

QHash<int, QByteArray> UserListModel::roleNames() const {
  return {
      {UserNameRole, QByteArrayLiteral("username")},
      {DisplayNameRole, QByteArrayLiteral("displayName")},
      {FullNameRole, QByteArrayLiteral("fullName")},
      {AvatarRole, QByteArrayLiteral("avatar")},
      {UidRole, QByteArrayLiteral("uid")},
      {LoggedInRole, QByteArrayLiteral("loggedIn")},
      {LockedRole, QByteArrayLiteral("locked")},
      {LockMinutesRole, QByteArrayLiteral("lockMinutes")},
      {IsCurrentRole, QByteArrayLiteral("isCurrent")},
      {UserTypeRole, QByteArrayLiteral("userType")},
  };
}

int UserListModel::indexOfUid(uint uid) const {
  for (int i = 0; i < users_.size(); ++i) {
    if (users_.at(i) && users_.at(i)->uid() == uid) return i;
  }
  return -1;
}

int UserListModel::currentUserIndex() const {
  if (!model_ || !model_->currentUser()) return -1;
  return indexOfUid(model_->currentUser()->uid());
}

std::shared_ptr<User> UserListModel::userAt(int index) const {
  if (index < 0 || index >= users_.size()) return std::shared_ptr<User>();
  return users_.at(index);
}

void UserListModel::refresh() {
  if (!model_) return;

  const QList<std::shared_ptr<User>> users = model_->userList();
  if (users.size() == users_.size()) {
    bool same = true;
    for (int i = 0; i < users.size(); ++i) {
      if (users.at(i) != users_.at(i)) {
        same = false;
        break;
      }
    }
    if (same) {
      if (!users_.isEmpty()) {
        emit dataChanged(index(0), index(static_cast<int>(users_.size()) - 1));
      }
      return;
    }
  }

  beginResetModel();
  disconnectUsers();
  users_ = users;
  for (const std::shared_ptr<User>& user : users_) {
    connectUser(user);
  }
  endResetModel();
}

void UserListModel::onUserListChanged(
    const QList<std::shared_ptr<User>>& users) {
  Q_UNUSED(users)
  refresh();
}

void UserListModel::onCurrentUserChanged(const std::shared_ptr<User>& user) {
  Q_UNUSED(user)
  if (users_.isEmpty()) return;
  emit dataChanged(index(0), index(static_cast<int>(users_.size()) - 1),
                   {IsCurrentRole});
}

void UserListModel::disconnectUsers() {
  for (auto it = user_connections_.begin(); it != user_connections_.end();
       ++it) {
    for (const QMetaObject::Connection& connection : it.value()) {
      QObject::disconnect(connection);
    }
  }
  user_connections_.clear();
}

void UserListModel::connectUser(const std::shared_ptr<User>& user) {
  if (!user) return;

  User* raw = user.get();
  QList<QMetaObject::Connection> connections;
  connections << connect(raw, &User::displayNameChanged, this,
                         [this, raw](const QString&) { notifyUserChanged(raw); });
  connections << connect(raw, &User::avatarChanged, this,
                         [this, raw](const QString&) { notifyUserChanged(raw); });
  connections << connect(raw, &User::lockChanged, this,
                         [this, raw](bool) { notifyUserChanged(raw); });
  connections << connect(raw, &User::logindChanged, this,
                         [this, raw](bool) { notifyUserChanged(raw); });
  connections << connect(raw, &User::lockBackgroundPathChanged, this,
                         [this, raw](const QString&) { notifyUserChanged(raw); });
  connections << connect(raw, &User::kbLayoutListChanged, this,
                         [this, raw](const QStringList&) { notifyUserChanged(raw); });
  connections << connect(raw, &User::currentKBLayoutChanged, this,
                         [this, raw](const QString&) { notifyUserChanged(raw); });
  user_connections_.insert(raw, connections);
}

void UserListModel::notifyUserChanged(User* user) {
  for (int i = 0; i < users_.size(); ++i) {
    if (users_.at(i).get() == user) {
      emit dataChanged(index(i), index(i));
      return;
    }
  }
}

}  // namespace lock
}  // namespace gxde
