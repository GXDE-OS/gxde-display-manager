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

#ifndef GXDE_DISPLAY_MANAGER_GREETER_NEO_SRC_SESSION_LIST_H_
#define GXDE_DISPLAY_MANAGER_GREETER_NEO_SRC_SESSION_LIST_H_

#include <QStringList>
#include <QVariantList>

namespace gxde::greeter {

QVariantList ReadSessionDirectories(const QStringList& x11_directories,
                                    const QStringList& wayland_directories);

QVariantList ReadSessions(const QStringList& data_directories);

}

#endif
