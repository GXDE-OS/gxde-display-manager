/***************************************************************************
* Copyright (c) 2023 Fabian Vogt <fvogt@suse.de>
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

#include "Session.h"

#include <QFile>
#include <QLocale>
#include <QTemporaryDir>
#include <QTest>

class SessionTest : public QObject {
    Q_OBJECT
private slots:
    void testCLocale()
    {
        QLocale::setDefault(QLocale::c());
        auto fileName = QFINDTESTDATA("plasmawayland-dev.desktop");
        SDDM::Session session(SDDM::Session::WaylandSession, fileName);
        QVERIFY(session.isValid());
        QCOMPARE(session.xdgSessionType(), QStringLiteral("wayland"));
        QCOMPARE(session.fileName(), fileName);
        QCOMPARE(session.displayName(), QStringLiteral("Plasma (Development, Wayland /usr/bin)"));
        QCOMPARE(session.comment(), QStringLiteral("Plasma by KDE"));
        QCOMPARE(session.exec(), QStringLiteral("/usr/lib64/libexec/plasma-dbus-run-session-if-needed /usr/lib64/libexec/startplasma-dev.sh -wayland"));
        QCOMPARE(session.tryExec(), QString());
        QCOMPARE(session.desktopSession(), QStringLiteral("plasmawayland-dev"));
        QCOMPARE(session.desktopNames(), QStringLiteral("KDE"));
        QCOMPARE(session.isHidden(), false);
        QCOMPARE(session.isNoDisplay(), false);
    }
    void testKOLocale()
    {
        QLocale::setDefault(QLocale{QStringLiteral("ko_KO")});
        auto fileName = QFINDTESTDATA("plasmawayland-dev.desktop");
        SDDM::Session session(SDDM::Session::WaylandSession, fileName);
        QVERIFY(session.isValid());
        QCOMPARE(session.xdgSessionType(), QStringLiteral("wayland"));
        QCOMPARE(session.fileName(), fileName);
        QCOMPARE(session.displayName(), QStringLiteral("Plasma(\uAC1C\uBC1C, Wayland /usr/bin)"));
        QCOMPARE(session.comment(), QStringLiteral("KDE Plasma"));
        QCOMPARE(session.exec(), QStringLiteral("/usr/lib64/libexec/plasma-dbus-run-session-if-needed /usr/lib64/libexec/startplasma-dev.sh -wayland"));
        QCOMPARE(session.tryExec(), QString());
        QCOMPARE(session.desktopSession(), QStringLiteral("plasmawayland-dev"));
        QCOMPARE(session.desktopNames(), QStringLiteral("KDE"));
        QCOMPARE(session.isHidden(), false);
        QCOMPARE(session.isNoDisplay(), false);
    }

    void testAvailability()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        auto writeDesktop = [](const QString &path, const QByteArray &extra) {
            QFile file(path);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
                return false;

            file.write("[Desktop Entry]\n");
            file.write("Name=Test Session\n");
            file.write("Exec=/bin/sh\n");
            file.write(extra);
            return true;
        };

        const QString availableFile = dir.filePath(QStringLiteral("available.desktop"));
        QVERIFY(writeDesktop(availableFile, QByteArray()));
        SDDM::Session available(SDDM::Session::WaylandSession, availableFile);
        QVERIFY(available.isValid());
        QVERIFY(available.isAvailable());

        const QString hiddenFile = dir.filePath(QStringLiteral("hidden.desktop"));
        QVERIFY(writeDesktop(hiddenFile, QByteArrayLiteral("Hidden=true\n")));
        SDDM::Session hidden(SDDM::Session::WaylandSession, hiddenFile);
        QVERIFY(hidden.isValid());
        QVERIFY(!hidden.isAvailable());

        const QString noDisplayFile = dir.filePath(QStringLiteral("nodisplay.desktop"));
        QVERIFY(writeDesktop(noDisplayFile, QByteArrayLiteral("NoDisplay=true\n")));
        SDDM::Session noDisplay(SDDM::Session::WaylandSession, noDisplayFile);
        QVERIFY(noDisplay.isValid());
        QVERIFY(!noDisplay.isAvailable());

        const QString badTryExecFile = dir.filePath(QStringLiteral("badtryexec.desktop"));
        QVERIFY(writeDesktop(badTryExecFile, QByteArrayLiteral("TryExec=/no/such/gxdm-session-test\n")));
        SDDM::Session badTryExec(SDDM::Session::WaylandSession, badTryExecFile);
        QVERIFY(badTryExec.isValid());
        QVERIFY(!badTryExec.isAvailable());

        const QString missingExecFile = dir.filePath(QStringLiteral("missingexec.desktop"));
        QFile missingExec(missingExecFile);
        QVERIFY(missingExec.open(QIODevice::WriteOnly | QIODevice::Truncate));
        missingExec.write("[Desktop Entry]\n");
        missingExec.write("Name=Missing Exec\n");
        missingExec.close();

        SDDM::Session missingExecSession(SDDM::Session::WaylandSession, missingExecFile);
        QVERIFY(missingExecSession.isValid());
        QVERIFY(!missingExecSession.isAvailable());
    }
};

QTEST_MAIN(SessionTest);

#include "SessionTest.moc"
