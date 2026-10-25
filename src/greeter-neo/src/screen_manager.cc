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

#include "greeter-neo/src/screen_manager.h"

#include <QCoreApplication>
#include <QCursor>
#include <QDebug>
#include <QEvent>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLatin1String>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRect>
#include <QScreen>

namespace gxde {
namespace greeter {
namespace {

bool IsX11() {
  return QGuiApplication::platformName() == QLatin1String("xcb");
}

QQuickItem* FocusItem(QQuickWindow* window) {
  if (QQuickItem* item = window->activeFocusItem()) {
    return item;
  }

  QQuickItem* item = window->contentItem();
  while (item && item->isFocusScope() && item->scopedFocusItem()) {
    item = item->scopedFocusItem();
  }

  return item;
}

}

ScreenManager::ScreenManager(bool windowed, QObject* parent)
    : QObject(parent), windowed_(windowed) {}

ScreenManager::~ScreenManager() {
  qApp->removeEventFilter(this);
  current_ = nullptr;
  const QList<QQuickWindow*> windows = windows_.values();
  windows_.clear();
  qDeleteAll(windows);
}

void ScreenManager::Start(QQmlComponent* component) {
  if (!component || component_) return;
  component_ = component;
  qApp->installEventFilter(this);

  if (windowed_) {
    AddScreen(QGuiApplication::primaryScreen());
  } else {
    connect(
        qApp, &QGuiApplication::screenAdded, this,
        [this](QScreen* screen) {
          AddScreen(screen);
          if (!current_) SetCurrentWindow(InitialWindow());
        },
        Qt::QueuedConnection);

    connect(qApp, &QGuiApplication::screenRemoved, this,
            &ScreenManager::RemoveScreen, Qt::DirectConnection);

    for (QScreen* screen : QGuiApplication::screens()) {
      AddScreen(screen);
    }
  }

  SetCurrentWindow(InitialWindow());
}

QQuickWindow* ScreenManager::CurrentWindow() const {
  return current_;
}

QList<QQuickWindow*> ScreenManager::Windows() const {
  return windows_.values();
}

bool ScreenManager::eventFilter(QObject* watched, QEvent* event) {
  switch (event->type()) {
    case QEvent::Enter:
    case QEvent::MouseMove:
      if (QQuickWindow* window = ManagedWindow(watched)) {
        SetCurrentWindow(window);
      }

      break;
    case QEvent::KeyPress:
    case QEvent::KeyRelease: {
      QQuickWindow* window = ManagedWindow(watched);
      if (!window || !current_ || window == current_) {
        break;
      }

      for (QQuickItem* item = FocusItem(current_); item;
           item = item->parentItem()) {
        event->accept();
        QCoreApplication::sendEvent(item, event);
        if (event->isAccepted()) break;
      }

      if (event->type() == QEvent::KeyPress &&
          static_cast<QKeyEvent*>(event)->matches(QKeySequence::Cancel)) {
        emit CancelForwarded();
      }
      return true;
    }
    default:
      break;
  }
  return QObject::eventFilter(watched, event);
}

void ScreenManager::AddScreen(QScreen* screen) {
  // A queued addition may name a screen that is already gone.
  if (!component_ || !screen || windows_.contains(screen) ||
      !QGuiApplication::screens().contains(screen)) {
    return;
  }

  QObject* object = component_->create(component_->creationContext());
  auto* window = qobject_cast<QQuickWindow*>(object);
  if (!window) {
    qWarning() << "Cannot create greeter window:" << component_->errors();
    delete object;
    return;
  }
  QQmlEngine::setObjectOwnership(window, QQmlEngine::CppOwnership);
  windows_.insert(screen, window);

  // Wayland compositors place a fullscreen surface on the output of its
  // screen, so bind it before the native window exists.
  window->setScreen(screen);
  if (windowed_) {
    window->show();
    return;
  }

  // Without a window manager nothing moves an X11 window onto its output.
  window->setGeometry(screen->geometry());
  if (IsX11()) {
    connect(screen, &QScreen::geometryChanged, window,
            [window](const QRect& geometry) { window->setGeometry(geometry); });
  }
  window->showFullScreen();
}

void ScreenManager::RemoveScreen(QScreen* screen) {
  QQuickWindow* window = windows_.take(screen);
  if (!window) return;
  // Move the login scene out before its window goes away.
  if (window == current_) SetCurrentWindow(InitialWindow());
  delete window;
}

void ScreenManager::SetCurrentWindow(QQuickWindow* window) {
  if (window == current_) return;
  current_ = window;
  emit CurrentWindowChanged();
}

QQuickWindow* ScreenManager::ManagedWindow(QObject* object) const {
  if (!object->isWindowType()) return nullptr;
  for (QQuickWindow* window : windows_) {
    if (window == object) return window;
  }
  return nullptr;
}

QQuickWindow* ScreenManager::InitialWindow() const {
  if (windows_.isEmpty()) return nullptr;
  // Only X11 knows where the pointer is before it enters one of our windows.
  if (IsX11()) {
    QScreen* screen = QGuiApplication::screenAt(QCursor::pos());
    if (windows_.contains(screen)) return windows_.value(screen);
  }
  return windows_.value(QGuiApplication::primaryScreen(), windows_.first());
}

}  // namespace greeter
}  // namespace gxde
