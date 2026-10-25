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

#include <QByteArray>
#include <QCoreApplication>
#include <QEnterEvent>
#include <QGuiApplication>
#include <QObject>
#include <QPointF>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QScreen>
#include <QSet>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QTest>

namespace gxde::greeter {
namespace {

constexpr char kWindowQml[] = R"(
import QtQuick

Window {
  id: window
  property int keys: 0
  Item {
    focus: true
    // Leaves Escape to the popups, like the greeter's own items.
    Keys.onPressed: (event) => {
      window.keys++
      event.accepted = event.key !== Qt.Key_Escape
    }
  }
}
)";

class ScreenManagerTest : public QObject {
  Q_OBJECT

 private slots:
  void init();
  void cleanup();
  void ShowsWindowOnEveryScreen();
  void FollowsPointer();
  void ForwardsKeysToCurrentWindow();
  void PreviewUsesOneWindow();

 private:
  QQuickWindow* WindowOn(const ScreenManager& manager, QScreen* screen) const;

  QQmlEngine* engine_ = nullptr;
  QQmlComponent* component_ = nullptr;
};

void ScreenManagerTest::init() {
  engine_ = new QQmlEngine;
  component_ = new QQmlComponent(engine_);
  component_->setData(kWindowQml, QUrl());
  QVERIFY2(component_->isReady(), qPrintable(component_->errorString()));
}

void ScreenManagerTest::cleanup() {
  delete component_;
  delete engine_;
}

QQuickWindow* ScreenManagerTest::WindowOn(const ScreenManager& manager,
                                          QScreen* screen) const {
  for (QQuickWindow* window : manager.Windows()) {
    if (window->screen() == screen) return window;
  }
  return nullptr;
}

void ScreenManagerTest::ShowsWindowOnEveryScreen() {
  QCOMPARE(QGuiApplication::screens().size(), 2);
  ScreenManager manager(false);
  manager.Start(component_);

  QCOMPARE(manager.Windows().size(), 2);
  for (QScreen* screen : QGuiApplication::screens()) {
    QQuickWindow* window = WindowOn(manager, screen);
    QVERIFY(window);
    QVERIFY(window->isVisible());
    QCOMPARE(window->windowStates(), Qt::WindowFullScreen);
    QCOMPARE(window->geometry(), screen->geometry());
  }
  QCOMPARE(manager.CurrentWindow(),
           WindowOn(manager, QGuiApplication::primaryScreen()));
}

void ScreenManagerTest::FollowsPointer() {
  ScreenManager manager(false);
  manager.Start(component_);
  QQuickWindow* first = WindowOn(manager, QGuiApplication::screens().at(0));
  QQuickWindow* second = WindowOn(manager, QGuiApplication::screens().at(1));
  QSignalSpy changed(&manager, &ScreenManager::CurrentWindowChanged);

  QEnterEvent enter(QPointF(10, 10), QPointF(10, 10), QPointF(810, 10));
  QCoreApplication::sendEvent(second, &enter);
  QCOMPARE(manager.CurrentWindow(), second);
  QCOMPARE(changed.size(), 1);

  QTest::mouseMove(first, QPoint(20, 20));
  QTRY_COMPARE(manager.CurrentWindow(), first);
  QCOMPARE(changed.size(), 2);
}

void ScreenManagerTest::ForwardsKeysToCurrentWindow() {
  ScreenManager manager(false);
  manager.Start(component_);
  QQuickWindow* first = WindowOn(manager, QGuiApplication::screens().at(0));
  QQuickWindow* second = WindowOn(manager, QGuiApplication::screens().at(1));
  first->requestActivate();
  QVERIFY(QTest::qWaitForWindowActive(first));

  QEnterEvent enter(QPointF(10, 10), QPointF(10, 10), QPointF(810, 10));
  QCoreApplication::sendEvent(second, &enter);
  QCOMPARE(manager.CurrentWindow(), second);

  QTest::keyClick(first, Qt::Key_A);
  QCOMPARE(second->property("keys").toInt(), 1);
  QCOMPARE(first->property("keys").toInt(), 0);
  QCOMPARE(QGuiApplication::focusWindow(), first);

  QSignalSpy cancel(&manager, &ScreenManager::CancelForwarded);
  QTest::keyClick(first, Qt::Key_Escape);
  QCOMPARE(cancel.size(), 1);
  QCOMPARE(second->property("keys").toInt(), 2);
}

void ScreenManagerTest::PreviewUsesOneWindow() {
  ScreenManager manager(true);
  manager.Start(component_);

  QCOMPARE(manager.Windows().size(), 1);
  QQuickWindow* window = manager.Windows().constFirst();
  QCOMPARE(window->screen(), QGuiApplication::primaryScreen());
  QVERIFY(window->isVisible());
  QVERIFY(!(window->windowStates() & Qt::WindowFullScreen));
  QCOMPARE(manager.CurrentWindow(), window);
}

}
}

int main(int argc, char* argv[]) {
  // Two side by side screens on the offscreen platform.
  QTemporaryFile config;
  if (!config.open()) return 1;
  config.write(R"({"screens": [
    {"name": "left", "x": 0, "y": 0, "width": 800, "height": 600},
    {"name": "right", "x": 800, "y": 0, "width": 1024, "height": 768}
  ]})");
  config.close();
  qputenv("QT_QPA_PLATFORM",
          "offscreen:configfile=" + config.fileName().toLocal8Bit());

  QGuiApplication app(argc, argv);
  gxde::greeter::ScreenManagerTest test;
  return QTest::qExec(&test, argc, argv);
}

#include "screen_manager_test.moc"
