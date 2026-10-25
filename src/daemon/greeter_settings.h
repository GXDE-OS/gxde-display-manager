/***************************************************************************
 * Copyright (c) 2013 Abdurrahman AVCI <abdurrahmanavci@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 ***************************************************************************/

#ifndef GXDE_DISPLAY_MANAGER_DAEMON_GREETER_SETTINGS_H_
#define GXDE_DISPLAY_MANAGER_DAEMON_GREETER_SETTINGS_H_

#include <QString>

namespace gxde {

QString GreeterExecutable(bool neo_enabled, const QString& bin_directory,
                          int qt_major);

bool SaveGeneralSetting(const QString& config_path, const QString& key,
                        const QString& value);

}

#endif
