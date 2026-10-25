#ifndef SRC_COMMON_GREETERCURSOR_H_
#define SRC_COMMON_GREETERCURSOR_H_

#include <QtGlobal>

// Cursor setup shared by gxdm-greeter-classic and gxdm-greeter-neo.
namespace GxdmGreeterCursor {

// Exports the cursor theme chosen through GXDM's SetCursor (state.conf) as
// XCURSOR_THEME. Call before the QGuiApplication is created.
void applyConfiguredTheme();

// Keeps Qt from handing cursors to a Wayland compositor through
// wp_cursor_shape_manager_v1, so the configured theme is drawn by the
// client. Call before the QGuiApplication is created.
void enforceWaylandCursorConfig();

// Sets the X11 root window cursor to the login spinner (or watch), which
// stays up between the greeter exiting and the session mapping its windows.
bool setRootWindowWaitCursor(qreal devicePixelRatio);

}  // namespace GxdmGreeterCursor

#endif  // SRC_COMMON_GREETERCURSOR_H_
