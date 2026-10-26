#ifndef GXDE_GREETER_NEO_LOGIN_TRANSITION_H_
#define GXDE_GREETER_NEO_LOGIN_TRANSITION_H_

#include <QHash>
#include <QObject>
#include <QTimer>

class QQuickWindow;

namespace gxde::greeter {

class GreeterBackend;
class ScreenManager;

// Waits for the animation's final frame before allowing the session to start.
// Each transition owns a connection context, so cancellation also discards
// queued callbacks from its windows.
class LoginTransition : public QObject {
 public:
  LoginTransition(GreeterBackend& backend, ScreenManager& screens);

 private:
  void Start(bool preserve_background);
  void Cancel();
  void WatchWindow(QQuickWindow* window);
  void FrameSwapped(QQuickWindow* window);
  void WindowFinished(QQuickWindow* window);
  void Complete(bool frames_ready);

  GreeterBackend& backend_;
  ScreenManager& screens_;
  QTimer frame_timeout_;
  QObject* frame_connections_ = nullptr;
  QHash<QQuickWindow*, int> pending_frames_;
  bool preserve_background_ = false;
};

}  // namespace gxde::greeter

#endif  // GXDE_GREETER_NEO_LOGIN_TRANSITION_H_
