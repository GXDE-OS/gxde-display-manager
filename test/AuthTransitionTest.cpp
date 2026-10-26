#include "Auth.h"
#include "AuthMessages.h"
#include "SafeDataStream.h"

#include <QCoreApplication>
#include <QLocalSocket>
#include <QSignalSpy>
#include <QTest>

using namespace SDDM;

// The same binary acts as a harmless helper. No PAM, VT, or user session is used.
static int runHelper(const QStringList &args) {
    QLocalSocket socket;
    const int socketArgument = args.indexOf(QStringLiteral("--socket")) + 1;
    const int idArgument = args.indexOf(QStringLiteral("--id")) + 1;

    socket.connectToServer(args.value(socketArgument));
    if (!socket.waitForConnected(2000)) {
        return 1;
    }

    SafeDataStream stream(&socket);
    stream << HELLO << args.value(idArgument).toLongLong();
    stream.send();
    stream << AUTHENTICATED << QStringLiteral("test-user");
    stream.send();
    stream.receive();

    Msg message;
    QProcessEnvironment environment;
    QByteArray cookie;
    stream >> message >> environment >> cookie;
    if (message != AUTHENTICATED || cookie != QByteArrayLiteral("test-cookie")) {
        return 2;
    }

    stream.reset();
    stream << SESSION_STATUS << true;
    stream.send();
    stream.receive();
    stream >> message;
    return message == SESSION_STATUS ? 0 : 3;
}

class AuthTransitionTest : public QObject {
    Q_OBJECT

private slots:
    void reply_data() {
        QTest::addColumn<bool>("deferred");
        QTest::newRow("legacy-immediate") << false;
        QTest::newRow("transition-deferred") << true;
    }

    void reply() {
        QFETCH(bool, deferred);

        Auth auth;
        QSignalSpy authenticated(&auth, &Auth::authentication);
        QSignalSpy started(&auth, &Auth::sessionStarted);
        QSignalSpy finished(&auth, &Auth::finished);

        connect(&auth, &Auth::authentication, &auth, [&] {
            auth.setCookie(QByteArrayLiteral("test-cookie"));
            if (deferred) {
                auth.deferSessionStart();
            }
        });

        auth.start();
        QTRY_COMPARE(authenticated.size(), 1);
        if (deferred) {
            QTest::qWait(60);
            QCOMPARE(started.size(), 0);
            QVERIFY(auth.isActive());
            auth.resumeSessionStart();
            auth.resumeSessionStart(); // Never send a duplicate reply.
        }
        QTRY_COMPARE(started.size(), 1);
        QVERIFY(started.first().first().toBool());
        QTRY_COMPARE(finished.size(), 1);
        QCOMPARE(finished.first().first().value<Auth::HelperExitStatus>(), Auth::HELPER_SUCCESS);
    }

    void cancelThenRetry() {
        Auth auth;
        bool defer = true;
        QSignalSpy authenticated(&auth, &Auth::authentication);
        QSignalSpy started(&auth, &Auth::sessionStarted);

        connect(&auth, &Auth::authentication, &auth, [&] {
            auth.setCookie(QByteArrayLiteral("test-cookie"));
            if (defer) {
                auth.deferSessionStart();
            }
        });

        auth.start();
        QTRY_COMPARE(authenticated.size(), 1);
        auth.stop();
        auth.resumeSessionStart();
        QCOMPARE(started.size(), 0);

        defer = false;
        auth.start();
        QTRY_COMPARE(started.size(), 1);
        QTRY_VERIFY(!auth.isActive());
    }
};

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    if (app.arguments().contains(QStringLiteral("--socket"))) {
        return runHelper(app.arguments());
    }

    Auth::registerTypes();
    AuthTransitionTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "AuthTransitionTest.moc"
