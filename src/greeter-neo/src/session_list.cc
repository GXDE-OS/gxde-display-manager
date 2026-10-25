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

#include "greeter-neo/src/session_list.h"

#include <QDir>
#include <QFile>
#include <QHash>
#include <QLocale>
#include <QSet>
#include <QStandardPaths>
#include <QString>
#include <QVariantMap>
#include <algorithm>

namespace gxde::greeter {
namespace {

QHash<QString, QString> ReadDesktopEntry(const QString& path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return {};
  }

  QHash<QString, QString> entry;
  bool in_entry = false;
  while (!file.atEnd()) {
    const QString line = QString::fromUtf8(file.readLine()).trimmed();
    if (line.startsWith('[')) {
      in_entry = line == "[Desktop Entry]";
      continue;
    }

    const auto separator = line.indexOf('=');
    if (!in_entry || line.startsWith('#') || separator <= 0) {
      continue;
    }

    const QString raw = line.mid(separator + 1);
    QString value;
    for (qsizetype i = 0; i < raw.size(); ++i) {
      if (raw.at(i) == '\\' && i + 1 < raw.size()) {
        const QChar next = raw.at(++i);
        if (next == 's')
          value += ' ';
        else if (next == 'n')
          value += '\n';
        else if (next == 't')
          value += '\t';
        else if (next == 'r')
          value += '\r';
        else if (next == '\\')
          value += '\\';
        else {
          value += '\\';
          value += next;
        }
      } else {
        value += raw.at(i);
      }
    }
    entry.insert(line.left(separator).trimmed(), value);
  }

  return entry;
}

}

QVariantList ReadSessions(const QStringList& data_directories) {
  QStringList x11_directories;
  QStringList wayland_directories;
  for (const QString& directory : data_directories) {
    x11_directories.append(QDir(directory).filePath("xsessions"));
    wayland_directories.append(QDir(directory).filePath("wayland-sessions"));
  }
  return ReadSessionDirectories(x11_directories, wayland_directories);
}

QVariantList ReadSessionDirectories(const QStringList& x11_directories,
                                    const QStringList& wayland_directories) {
  QVariantList sessions;
  QSet<QString> seen;
  for (const QString& type :
       {QStringLiteral("x11"), QStringLiteral("wayland")}) {
    const auto& directories =
        type == "x11" ? x11_directories : wayland_directories;
    for (const QString& directory : directories) {
      const QDir session_directory(directory);
      for (const QString& file : session_directory.entryList(
               {"*.desktop"}, QDir::Files | QDir::Readable, QDir::Name)) {
        const QString id = type + '/' + file;
        if (seen.contains(id)) {
          continue;
        }

        seen.insert(id);
        const auto entry = ReadDesktopEntry(session_directory.filePath(file));
        if (entry.value("Hidden").compare("true", Qt::CaseInsensitive) == 0 ||
            entry.value("NoDisplay").compare("true", Qt::CaseInsensitive) ==
                0 ||
            entry.value("Exec").isEmpty()) {
          continue;
        }

        const QString try_exec = entry.value("TryExec");
        if (!try_exec.isEmpty() &&
            QStandardPaths::findExecutable(try_exec).isEmpty()) {
          continue;
        }

        QString name;
        for (QString locale : QLocale().uiLanguages()) {
          locale.replace('-', '_');
          name = entry.value("Name[" + locale + ']');
          if (name.isEmpty()) {
            name = entry.value("Name[" + locale.section('_', 0, 0) + ']');
          }

          if (!name.isEmpty()) {
            break;
          }
        }

        if (name.isEmpty()) {
          name = entry.value("Name");
        }

        if (name.isEmpty()) {
          continue;
        }

        sessions.append(QVariantMap{
            {"id", id}, {"name", name}, {"type", type}, {"file", file}});
      }
    }
  }

  std::sort(sessions.begin(), sessions.end(),
            [](const QVariant& left, const QVariant& right) {
              const auto a = left.toMap();
              const auto b = right.toMap();
              const int order = a.value("name").toString().localeAwareCompare(
                  b.value("name").toString());
              return order == 0
                         ? a.value("id").toString() < b.value("id").toString()
                         : order < 0;
            });
  return sessions;
}

}
