/***************************************************************************
* Copyright (c) 2013 Abdurrahman AVCI <abdurrahmanavci@gmail.com>
* Copyright (c) 2014 David Edmundson <davidedmundson@kde.org>
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

#ifndef SDDM_UTILS_H
#define SDDM_UTILS_H

#include <QDir>
#include <QString>
#include <QStringList>
#include <QStandardPaths>

#include <random>

namespace SDDM {

inline QString generateName(int length) {
    const QString digits = QStringLiteral("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");

    // reserve space for name
    QString name;
    name.resize(length);

    // create random device
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, digits.length() - 1);

    // generate name
    for (int i = 0; i < length; ++i)
        name[i] = digits.at(dis(gen));

    // return result
    return name;
}

inline QString resolveCursorTheme(const QString& configuredTheme) {
    const QString requestedTheme = configuredTheme.trimmed();
    if (!requestedTheme.isEmpty())
        return requestedTheme;

    QStringList searchPaths = qEnvironmentVariable("XCURSOR_PATH")
        .split(QLatin1Char(':'), Qt::SkipEmptyParts);
    for (const QString &dataPath : QStandardPaths::standardLocations(
            QStandardPaths::GenericDataLocation)) {
        searchPaths.append(dataPath + QStringLiteral("/icons"));
    }

    searchPaths.append(QStringLiteral("/usr/share/pixmaps"));
    searchPaths.removeDuplicates();

    const QStringList preferredThemes = {
        QStringLiteral("gxde"),
        QStringLiteral("deepin"),
        QStringLiteral("Adwaita"),
    };

    for (const QString &theme : preferredThemes) {
        for (const QString &path : searchPaths) {
            if (QDir(path + QLatin1Char('/') + theme + QStringLiteral(
                    "/cursors")).exists())
                return theme;
        }
    }

    return QString();
}
}

#endif
