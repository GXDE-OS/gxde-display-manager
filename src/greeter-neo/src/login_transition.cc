#include "login_transition.h"

#include <QGuiApplication>
#include <QQuickWindow>

#include "common/GreeterCursor.h"
#include "greeter_backend.h"
#include "screen_manager.h"

namespace gxde::greeter {
namespace {

constexpr int kFrameTimeoutMs = 300;

// The first swap may belong to a frame already in flight when QML finished.
constexpr int kRequiredFrameSwaps = 2;

}  // namespace

LoginTransition::LoginTransition(GreeterBackend& backend, ScreenManager& screens)
    : backend_(backend), screens_(screens) {
  frame_timeout_.setSingleShot(true);
  frame_timeout_.setInterval(kFrameTimeoutMs);

  connect(&backend_, &GreeterBackend::TransitionFinished,
          this, &LoginTransition::Start);
  connect(&backend_, &GreeterBackend::LoginFailed,
          this, &LoginTransition::Cancel);
  connect(&backend_, &GreeterBackend::LoginSucceeded,
          this, &LoginTransition::Cancel);
  connect(&backend_, &GreeterBackend::PrepareTransition,
          this, &LoginTransition::Cancel);

  // Stalled or hidden outputs must not block login indefinitely.
  connect(&frame_timeout_, &QTimer::timeout, this, [this] {
    Complete(false);
  });

  backend_.EnableTransitions();
}

void LoginTransition::Start(bool preserve_background) {
  Cancel();
  preserve_background_ = preserve_background;
  frame_connections_ = new QObject(this);

  for (auto* window : screens_.Windows()) {
    if (window->isExposed()) {
      WatchWindow(window);
    }
  }

  if (pending_frames_.isEmpty()) {
    Complete(true);
    return;
  }

  frame_timeout_.start();
}

void LoginTransition::Cancel() {
  frame_timeout_.stop();

  delete frame_connections_;
  frame_connections_ = nullptr;
  pending_frames_.clear();
}

void LoginTransition::WatchWindow(QQuickWindow* window) {
  pending_frames_.insert(window, 0);

  connect(window, &QQuickWindow::frameSwapped, frame_connections_,
          [this, window] {
            FrameSwapped(window);
          }, Qt::QueuedConnection);
  connect(window, &QObject::destroyed, frame_connections_,
          [this, window] {
            WindowFinished(window);
          });

  window->update();
}

void LoginTransition::FrameSwapped(QQuickWindow* window) {
  auto frame_count = pending_frames_.find(window);
  if (frame_count == pending_frames_.end()) {
    return;
  }

  ++frame_count.value();
  if (frame_count.value() < kRequiredFrameSwaps) {
    window->update();
    return;
  }

  WindowFinished(window);
}

void LoginTransition::WindowFinished(QQuickWindow* window) {
  pending_frames_.remove(window);
  if (pending_frames_.isEmpty()) {
    Complete(true);
  }
}

void LoginTransition::Complete(bool frames_ready) {
  const bool can_preserve_background = frames_ready && preserve_background_
      && backend_.TransitionPending()
      && QGuiApplication::platformName() == QLatin1String("xcb");

  if (can_preserve_background) {
    GxdmGreeterCursor::preserveRootBackground();
  }

  backend_.TransitionReady();
  Cancel();
}

}  // namespace gxde::greeter
