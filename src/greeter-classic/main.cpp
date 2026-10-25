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
#include "GreeterCursor.h"
#include "GreeterScale.h"
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
#include <X11/Xlib-xcb.h>

DCORE_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

int main(int argc, char* argv[])
{
    GxdmGreeterCursor::applyConfiguredTheme();

    QByteArray requestedPlatform = qgetenv("QT_QPA_PLATFORM");
    for (int i = 1; requestedPlatform.isEmpty() && i + 1 < argc; ++i) {
        if (QByteArray(argv[i]) == "-platform")
            requestedPlatform = argv[i + 1];
    }
    const bool useX11 = requestedPlatform.startsWith("xcb")
        || requestedPlatform.startsWith("dxcb")
        || (requestedPlatform.isEmpty() && !qEnvironmentVariableIsSet("WAYLAND_DISPLAY"));

    if (!useX11) {
        GxdmGreeterCursor::enforceWaylandCursorConfig();
    }

    // load dpi settings
    if (useX11 && !QFile::exists(GxdmGreeterScale::themeConfigPath())) {
        GxdmGreeterScale::applyAutoScreenScaleFactors();
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
            GxdmGreeterCursor::setRootWindowWaitCursor(qApp->devicePixelRatio());
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
