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

#ifndef SRC_COMMON_GREETERSCALE_H_
#define SRC_COMMON_GREETERSCALE_H_

#include <QByteArray>
#include <QPair>
#include <QString>

namespace GxdmGreeterScale {

QString themeConfigPath();

QPair<QByteArray, QByteArray> themeScaleEnvironment(
  const QString& path = QString());

void applyAutoScreenScaleFactors();

void applyX11ScaleFactors();

}

#endif
