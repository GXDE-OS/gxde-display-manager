#include "GreeterCursor.h"

#include "GreeterAppearance.h"

#include <QByteArrayList>
#include <QDebug>

#include <X11/Xcursor/Xcursor.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xfixes.h>

namespace {

XcursorImages* loadImages(const char* image, int size) {
  const QByteArray configuredTheme = qgetenv("XCURSOR_THEME");
  if (!configuredTheme.isEmpty()) {
    return XcursorLibraryLoadImages(image, configuredTheme.constData(), size);
  }

  const QByteArrayList preferredThemes = {"gxde", "deepin", "Adwaita"};
  for (const QByteArray& theme : preferredThemes) {
    if (XcursorImages* images =
            XcursorLibraryLoadImages(image, theme.constData(), size)) {
      return images;
    }
  }
  return nullptr;
}

Cursor loadCursor(Display* display, const char* name, int size) {
  XcursorImages* images = loadImages(name, size);
  if (!images) return 0;

  const Cursor cursor = XcursorImagesLoadCursor(display, images);
  XcursorImagesDestroy(images);
  return cursor;
}

}  // namespace

namespace GxdmGreeterCursor {

void applyConfiguredTheme() {
  const QString theme = GxdmGreeterAppearance::cursorTheme();
  if (!theme.isEmpty()) qputenv("XCURSOR_THEME", theme.toUtf8());
}

void enforceWaylandCursorConfig() {
  const QByteArray variable = "QT_WAYLAND_DISABLED_INTERFACES";
  QByteArrayList disabledInterfaces = qgetenv(variable.constData()).split(',');
  for (QByteArray& interface : disabledInterfaces) {
    interface = interface.trimmed();
  }
  disabledInterfaces.removeAll(QByteArray());

  const QByteArray cursorShapeInterface = "wp_cursor_shape_manager_v1";
  if (!disabledInterfaces.contains(cursorShapeInterface)) {
    disabledInterfaces.append(cursorShapeInterface);
  }
  qputenv(variable.constData(), disabledInterfaces.join(','));
}

bool setRootWindowWaitCursor(qreal devicePixelRatio) {
  Display* display = XOpenDisplay(nullptr);
  if (!display) {
    qDebug() << "Open display failed";
    return false;
  }

  Cursor cursor = loadCursor(display,
    devicePixelRatio > 1.7 ? "loginspinner@2x" : "loginspinner", 24);
  if (cursor == 0) cursor = loadCursor(display, "watch", 24);
  if (cursor == 0) {
    XCloseDisplay(display);
    return false;
  }

  XDefineCursor(display, XDefaultRootWindow(display), cursor);
  // XFixesChangeCursorByName is what actually changes the visible cursor;
  // XFreeCursor and XCloseDisplay are just as necessary.
  XFixesChangeCursorByName(display, cursor, "watch");
  XFreeCursor(display, cursor);
  XCloseDisplay(display);
  return true;
}

}  // namespace GxdmGreeterCursor
