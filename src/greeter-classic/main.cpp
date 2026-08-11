/*
 * Copyright (C) 2015 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     sbw <sbw@sbw.so>
 *             kirigaya <kirigaya@mkacg.com>
 *             Hualet <mr.asianwang@gmail.com>
 *
 * Maintainer: sbw <sbw@sbw.so>
 *             kirigaya <kirigaya@mkacg.com>
 *             Hualet <mr.asianwang@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "loginwindow.h"
#include "constants.h"
#include "greeterworker.h"
#include "greeterappearance.h"
#include "sessionbasemodel.h"
#include "propertygroup.h"
#include "multiscreenmanager.h"

#include <DApplication>
#include <QtCore/QTranslator>
#include <QLabel>
#include <QApplication>
#include <QProcess>
#include <QThread>
#include <QWindow>
#include <QScreen>
#include <DLog>

#include <cstdlib>
#include <memory>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <X11/Xlib-xcb.h>
#include <X11/cursorfont.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/extensions/Xfixes.h>

DCORE_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

static void enforceWaylandCursorConfig() {
    const QByteArray variable = "QT_WAYLAND_DISABLED_INTERFACES";
    QByteArrayList disabledInterfaces = qgetenv(
        variable.constData()).split(',');
    for (QByteArray &interface : disabledInterfaces) {
        interface = interface.trimmed();
    }

    disabledInterfaces.removeAll(QByteArray());

    const QByteArray cursorShapeInterface = "wp_cursor_shape_manager_v1";
    if (!disabledInterfaces.contains(cursorShapeInterface)) {
        disabledInterfaces.append(cursorShapeInterface);
    }

    qputenv(variable.constData(), disabledInterfaces.join(','));
}

//Load the System cursor --begin
static XcursorImages*
xcLoadImages(const char *image, int size) {
    const QByteArray configuredTheme = qgetenv("XCURSOR_THEME");
    if (!configuredTheme.isEmpty()) {
        return XcursorLibraryLoadImages(image,
            configuredTheme.constData(), size);
    }

    const QByteArrayList preferredThemes = {"gxde", "deepin", "Adwaita"};
    for (const QByteArray &theme : preferredThemes) {
        if (XcursorImages* images = XcursorLibraryLoadImages(
                image, theme.constData(), size)) {
            return images;
        }
    }

    return nullptr;
}

static unsigned long loadCursorHandle(Display *dpy, const char *name, int size)
{
    if (size == -1) {
        size = XcursorGetDefaultSize(dpy);
    }

    // Load the cursor images
    XcursorImages *images = NULL;
    images = xcLoadImages(name, size);

    if (!images) {
        return 0;
    }

    unsigned long handle = (unsigned long)XcursorImagesLoadCursor(dpy,
                          images);
    XcursorImagesDestroy(images);

    return handle;
}

static int set_rootwindow_cursor() {
    Display* display = XOpenDisplay(NULL);
    if (!display) {
        qDebug() << "Open display failed";
        return -1;
    }

    const char *cursorName = qApp->devicePixelRatio() > 1.7
        ? "loginspinner@2x"
        : "loginspinner";

    Cursor cursor = (Cursor)loadCursorHandle(display, cursorName, 24);
    if (cursor == 0) {
        cursor = (Cursor)loadCursorHandle(display, "watch", 24);
    }

    if (cursor == 0) {
        XCloseDisplay(display);
        return -1;
    }
    XDefineCursor(display, XDefaultRootWindow(display),cursor);

    // XFixesChangeCursorByName is the key to change the cursor
    // and the XFreeCursor and XCloseDisplay is also essential.

    XFixesChangeCursorByName(display, cursor, "watch");

    XFreeCursor(display, cursor);
    XCloseDisplay(display);

    return 0;
}
// Load system cursor --end

static double outputScaleRatio(const XRRCrtcInfo *crtcInfo, const XRROutputInfo *outputInfo)
{
    const double ratio = static_cast<double>(crtcInfo->width)
        / static_cast<double>(outputInfo->mm_width)
        / (1366.0 / 310.0);

    if (ratio > 1 + 2.0 / 3.0)
        return 2.0;
    if (ratio > 1 + 1.0 / 3.0)
        return 1.5;
    return 1.0;
}

static void set_auto_QT_SCREEN_SCALE_FACTORS()
{
    if (qEnvironmentVariableIsSet("QT_SCALE_FACTOR")
        || qEnvironmentVariableIsSet("QT_SCREEN_SCALE_FACTORS")) {
        return;
    }

    Display *display = XOpenDisplay(nullptr);
    if (!display) {
        qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "1");
        return;
    }

    XRRScreenResources *resources = XRRGetScreenResourcesCurrent(
        display, DefaultRootWindow(display));
    if (!resources) {
        resources = XRRGetScreenResources(display, DefaultRootWindow(display));
        qWarning() << "XRRGetScreenResourcesCurrent failed, using XRRGetScreenResources.";
    }

    QByteArrayList factors;
    if (resources) {
        for (int i = 0; i < resources->noutput; ++i) {
            XRROutputInfo *outputInfo = XRRGetOutputInfo(
                display, resources, resources->outputs[i]);
            if (!outputInfo)
                continue;

            if (outputInfo->crtc != 0 && outputInfo->mm_width > 0) {
                XRRCrtcInfo *crtcInfo = XRRGetCrtcInfo(
                    display, resources, outputInfo->crtc);
                if (crtcInfo) {
                    const QByteArray outputName(outputInfo->name, outputInfo->nameLen);
                    const double ratio = outputScaleRatio(crtcInfo, outputInfo);
                    factors.append(outputName + '=' + QByteArray::number(ratio));
                    XRRFreeCrtcInfo(crtcInfo);
                }
            }

            XRRFreeOutputInfo(outputInfo);
        }
        XRRFreeScreenResources(resources);
    }
    XCloseDisplay(display);

    if (factors.isEmpty()) {
        qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "1");
        return;
    }

    const QByteArray screenScaleFactors = factors.join(';');
    qInfo() << "Using per-screen scale factors:" << screenScaleFactors;
    qputenv("QT_SCREEN_SCALE_FACTORS", screenScaleFactors);
}

int main(int argc, char* argv[])
{
    const QString cursorTheme = GxdmGreeterAppearance::cursorTheme();
    if (!cursorTheme.isEmpty())
        qputenv("XCURSOR_THEME", cursorTheme.toUtf8());

    QByteArray requestedPlatform = qgetenv("QT_QPA_PLATFORM");
    for (int i = 1; requestedPlatform.isEmpty() && i + 1 < argc; ++i) {
        if (QByteArray(argv[i]) == "-platform")
            requestedPlatform = argv[i + 1];
    }
    const bool useX11 = requestedPlatform.startsWith("xcb")
        || requestedPlatform.startsWith("dxcb")
        || (requestedPlatform.isEmpty() && !qEnvironmentVariableIsSet("WAYLAND_DISPLAY"));

    if (!useX11) {
        enforceWaylandCursorConfig();
    }

    // load dpi settings
    if (useX11 && !QFile::exists("/etc/lightdm/deepin/qt-theme.ini")) {
        set_auto_QT_SCREEN_SCALE_FACTORS();
    }
    else if (useX11) {
        DApplication::customQtThemeConfigPath("/etc/lightdm/");
    }

    std::unique_ptr<QApplication> application;
    if (useX11) {
        DApplication::loadDXcbPlugin();
        application = std::make_unique<DApplication>(argc, argv);
    } else {
        application = std::make_unique<QApplication>(argc, argv);
    }
    QApplication &a = *application;
    qApp->setOrganizationName("deepin");
    qApp->setApplicationName("lightdm-deepin-greeter");
    qApp->setApplicationVersion("2015.1.0");
    qApp->setAttribute(Qt::AA_ForceRasterWidgets);

    DLogManager::registerConsoleAppender();

    QString greeterSocket;
    {
        const QStringList args = a.arguments();
        const int idx = args.indexOf(QStringLiteral("--socket"));
        if (idx >= 0 && idx + 1 < args.size())
            greeterSocket = args.at(idx + 1);
    }

    SessionBaseModel *model = new SessionBaseModel(SessionBaseModel::AuthType::LightdmType);
    GreeterWorker *worker = new GreeterWorker(model, greeterSocket);

    if (useX11) {
        QObject::connect(model, &SessionBaseModel::authFinished, model, [=] {
            set_rootwindow_cursor();
        });
    }

    PropertyGroup *property_group = new PropertyGroup(worker);

    property_group->addProperty("contentVisible");

    auto createFrame = [&] (QScreen *screen) -> QWidget* {
        LoginWindow *loginFrame = new LoginWindow(model);
        loginFrame->setScreen(screen);
        property_group->addObject(loginFrame);
        QObject::connect(loginFrame, &LoginWindow::requestSwitchToUser, worker, &GreeterWorker::switchToUser);
        QObject::connect(loginFrame, &LoginWindow::requestAuthUser, worker, &GreeterWorker::authUser);
        QObject::connect(loginFrame, &LoginWindow::requestSetLayout, worker, &GreeterWorker::setLayout);
        QObject::connect(worker, &GreeterWorker::requestUpdateBackground, loginFrame, static_cast<void (LoginWindow::*)(const QString &)>(&LoginWindow::updateBackground));
        QObject::connect(loginFrame, &LoginWindow::destroyed, property_group, &PropertyGroup::removeObject);
        if (useX11)
            loginFrame->show();
        else
            loginFrame->showFullScreen();
        return loginFrame;
    };

    MultiScreenManager multi_screen_manager;
    multi_screen_manager.register_for_mutil_screen(createFrame);
    QObject::connect(model, &SessionBaseModel::visibleChanged, &multi_screen_manager, &MultiScreenManager::startRaiseContentFrame);

    return a.exec();
}
