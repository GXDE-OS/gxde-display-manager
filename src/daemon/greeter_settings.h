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

// Path of gxdm's state file (~gxdm/state.conf), the one holding
// [Greeter] Wallpaper.
QString StateConfigPath(const QString& state_directory = QString());

// The global greeter wallpaper, but only when state.conf explicitly stores a
// non-empty [Greeter] Wallpaper. An unset entry usually falls back to the
// built-in default, which must not be forwarded to the new greeter: doing so
// hides the current user's lock/desktop wallpaper (the greeter is then forced
// to a wallpaper the user never picked). Returns an empty string whenever the
// greeter should resolve the per-user wallpaper itself.
QString ConfiguredGreeterWallpaper(const QString& state_config_path = QString());

bool SaveGeneralSetting(const QString& config_path, const QString& key,
                        const QString& value);

}

#endif
