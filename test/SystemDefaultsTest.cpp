#include "SystemDefaults.h"

#include <QFile>
#include <QTemporaryDir>
#include <QTest>

class SystemDefaultsTest : public QObject
{
    Q_OBJECT

private slots:
    void selectsFlakewmOnGxde();
    void matchesGxdeCaseInsensitively();
    void keepsRememberedSessionOnGxde();
    void leavesOtherSystemsUnspecified();
    void identifiesLegacyDefaultSessions();
    void recognisesFlakewmAsNonLegacy();

private:
    QString writeOsRelease(const QByteArray &contents);

    QTemporaryDir m_temporaryDir;
};

QString SystemDefaultsTest::writeOsRelease(const QByteArray &contents)
{
    const QString path = m_temporaryDir.filePath(QStringLiteral("os-release"));
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return QString();
    file.write(contents);
    file.close();
    return path;
}

void SystemDefaultsTest::selectsFlakewmOnGxde()
{
    const QString path = writeOsRelease("NAME=GXDE\nID=GXDE\n");
    QCOMPARE(GxdmSystemDefaults::operatingSystemId(path), QStringLiteral("GXDE"));
    QVERIFY(GxdmSystemDefaults::isGxdeOperatingSystem(path));
    QCOMPARE(GxdmSystemDefaults::defaultSessionDesktopFile(path),
        QStringLiteral("gxde-flakewm.desktop"));
}

void SystemDefaultsTest::matchesGxdeCaseInsensitively()
{
    const QString path = writeOsRelease("ID=gXdE\n");
    QVERIFY(GxdmSystemDefaults::isGxdeOperatingSystem(path));
    QCOMPARE(GxdmSystemDefaults::defaultSessionDesktopFile(path),
        QStringLiteral("gxde-flakewm.desktop"));
}

void SystemDefaultsTest::keepsRememberedSessionOnGxde()
{
    const QString path = writeOsRelease("ID=GXDE\n");
    QCOMPARE(GxdmSystemDefaults::initialSessionDesktopFile(
                 QStringLiteral("gxde-wlcom.desktop"), path),
        QStringLiteral("gxde-wlcom.desktop"));
}

void SystemDefaultsTest::leavesOtherSystemsUnspecified()
{
    const QString path = writeOsRelease("NAME=Debian GNU/Linux\nID=debian\n");
    QVERIFY(!GxdmSystemDefaults::isGxdeOperatingSystem(path));
    QCOMPARE(GxdmSystemDefaults::defaultSessionDesktopFile(path), QString());
}

void SystemDefaultsTest::identifiesLegacyDefaultSessions()
{
    QVERIFY(GxdmSystemDefaults::isLegacyDefaultSession(
        QStringLiteral("/usr/share/xsessions/deepin.desktop")));
    QVERIFY(GxdmSystemDefaults::isLegacyDefaultSession(
        QStringLiteral("gxde-wlcom.desktop")));
    QVERIFY(GxdmSystemDefaults::isLegacyDefaultSession(
        QStringLiteral("/usr/share/wayland-sessions/gxde-wlcom.desktop")));
    QVERIFY(GxdmSystemDefaults::isLegacyDefaultSession(
        QStringLiteral("DEEPIN.desktop")));
}

void SystemDefaultsTest::recognisesFlakewmAsNonLegacy()
{
    QVERIFY(!GxdmSystemDefaults::isLegacyDefaultSession(
        QStringLiteral("/usr/share/wayland-sessions/gxde-flakewm.desktop")));
    QVERIFY(!GxdmSystemDefaults::isLegacyDefaultSession(QString()));
    QVERIFY(!GxdmSystemDefaults::isLegacyDefaultSession(
        QStringLiteral("xfce.desktop")));
}

QTEST_MAIN(SystemDefaultsTest)
#include "SystemDefaultsTest.moc"
