#include "SessionReuse.h"

#include <QTest>

class SessionReuseTest : public QObject
{
    Q_OBJECT

private slots:
    void acceptsMatchingUserSession();
    void rejectsDifferentSessionType();
    void rejectsGreeterAndClosingSessions();
};

void SessionReuseTest::acceptsMatchingUserSession()
{
    QVERIFY(SDDM::canReuseLogindSession(
        QStringLiteral("gxdm"), QStringLiteral("online"),
        QStringLiteral("user"), QStringLiteral("wayland"),
        QStringLiteral("wayland")));
    QVERIFY(SDDM::canReuseLogindSession(
        QStringLiteral("gxdm-autologin"), QStringLiteral("online"),
        QStringLiteral("user"), QStringLiteral("x11"),
        QStringLiteral("x11")));
}

void SessionReuseTest::rejectsDifferentSessionType()
{
    QVERIFY(!SDDM::canReuseLogindSession(
        QStringLiteral("gxdm"), QStringLiteral("online"),
        QStringLiteral("user"), QStringLiteral("wayland"),
        QStringLiteral("x11")));
    QVERIFY(!SDDM::canReuseLogindSession(
        QStringLiteral("gxdm"), QStringLiteral("online"),
        QStringLiteral("user"), QStringLiteral("x11"),
        QStringLiteral("wayland")));
}

void SessionReuseTest::rejectsGreeterAndClosingSessions()
{
    QVERIFY(!SDDM::canReuseLogindSession(
        QStringLiteral("gxdm-greeter"), QStringLiteral("online"),
        QStringLiteral("greeter"), QStringLiteral("wayland"),
        QStringLiteral("wayland")));
    QVERIFY(!SDDM::canReuseLogindSession(
        QStringLiteral("gxdm"), QStringLiteral("closing"),
        QStringLiteral("user"), QStringLiteral("wayland"),
        QStringLiteral("wayland")));
}

QTEST_MAIN(SessionReuseTest)

#include "SessionReuseTest.moc"
