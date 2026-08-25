/***************************************************************************
* Copyright (c) 2026 CHarOfString <root@charofstring.cc>
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

#ifndef SRC_DAEMON_SESSIONREUSE_H_
#define SRC_DAEMON_SESSIONREUSE_H_

#include <QString>

namespace SDDM {

inline bool canReuseLogindSession(const QString &service,
        const QString &state,
        const QString &sessionClass,
        const QString &sessionType,
        const QString &requestedType) {
    return (service == QLatin1String("gxdm")
            || service == QLatin1String("gxdm-autologin"))
        && state == QLatin1String("online")
        && sessionClass == QLatin1String("user")
        && sessionType == requestedType;
}

}  // namespace SDDM

#endif  // SRC_DAEMON_SESSIONREUSE_H_
