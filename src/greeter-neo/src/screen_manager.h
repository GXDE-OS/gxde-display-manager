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

#ifndef GXDE_DISPLAY_MANAGER_GREETER_NEO_SRC_SCREEN_MANAGER_H_
#define GXDE_DISPLAY_MANAGER_GREETER_NEO_SRC_SCREEN_MANAGER_H_

#include <QList>
#include <QMap>
#include <QObject>
#include <QPointer>
#include <QQuickWindow>

class QEvent;
class QQmlComponent;
class QScreen;

namespace gxde::greeter {

// Shows a greeter window on every screen, like gxdm-greeter-classic's
// MultiScreenManager, and picks the one under the pointer to host the login
// scene. Key events the compositor sends to another greeter window are
// forwarded to that one, so typing works before it receives keyboard focus.
// Only public Qt API is used.
class ScreenManager : public QObject {
  Q_OBJECT
  Q_PROPERTY(QQuickWindow* currentWindow READ CurrentWindow NOTIFY
                 CurrentWindowChanged)

 public:
  // A windowed manager shows a single window on the primary screen.
  explicit ScreenManager(bool windowed, QObject* parent = nullptr);
  ~ScreenManager() override;

  // Creates a window from `component` for each screen, now and on hotplug.
  void Start(QQmlComponent* component);
  QQuickWindow* CurrentWindow() const;
  QList<QQuickWindow*> Windows() const;

 signals:
  void CurrentWindowChanged();
  // An Escape forwarded to the current window. A popup closes itself on
  // Escape only while it has active focus, which nothing has in a window the
  // compositor did not focus, so QML closes it instead.
  void CancelForwarded();

 protected:
  bool eventFilter(QObject* watched, QEvent* event) override;

 private:
  void AddScreen(QScreen* screen);
  void RemoveScreen(QScreen* screen);
  void SetCurrentWindow(QQuickWindow* window);
  QQuickWindow* ManagedWindow(QObject* object) const;
  QQuickWindow* InitialWindow() const;

  const bool windowed_;
  QPointer<QQmlComponent> component_;
  QMap<QScreen*, QQuickWindow*> windows_;
  QQuickWindow* current_ = nullptr;
};

}

#endif
