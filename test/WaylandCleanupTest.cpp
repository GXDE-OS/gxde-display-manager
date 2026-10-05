#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTemporaryDir>
#include <QTest>

#include <cerrno>
#include <memory>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>

class WaylandCleanupTest : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<QTemporaryDir> m_runtime;
    QProcess m_helper;

    static pid_t readPid(const QString &path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
            return -1;
        bool ok = false;
        const int pid = file.readAll().toInt(&ok);
        return ok && pid > 0 ? pid : -1;
    }

    static bool exited(pid_t pid)
    {
        // Reap adopted descendants so this check does not depend on PID 1.
        while (waitpid(pid, nullptr, WNOHANG) < 0 && errno == EINTR) {}
        return kill(pid, 0) < 0 && errno == ESRCH;
    }

private slots:
    void initTestCase()
    {
        if (getuid() == 0)
            QSKIP("The Wayland helper requires an unprivileged user");
        QVERIFY(prctl(PR_SET_CHILD_SUBREAPER, 1, 0, 0, 0) == 0);
    }

    void init()
    {
        m_runtime = std::make_unique<QTemporaryDir>();
        QVERIFY(m_runtime->isValid());
    }

    void cleanup()
    {
        // QtTest calls cleanup even when an assertion returns early.
        if (m_helper.state() != QProcess::NotRunning) {
            m_helper.kill();
            m_helper.waitForFinished(5000);
        }
        const QDir runtime(m_runtime->path());
        for (const QString &file : runtime.entryList({QStringLiteral("*.pid")}, QDir::Files)) {
            const pid_t pid = readPid(runtime.filePath(file));
            if (pid > 0) {
                kill(pid, SIGKILL);
                QTRY_VERIFY_WITH_TIMEOUT(exited(pid), 5000);
            }
        }
        m_runtime.reset();
    }

    void stopsCompositor_data()
    {
        QTest::addColumn<QString>("mode");
        QTest::addColumn<bool>("success");
        QTest::newRow("greeter-exit") << QStringLiteral("normal") << true;
        QTest::newRow("greeter-crash") << QStringLiteral("crash") << false;
        QTest::newRow("launcher-exits-with-live-child") << QStringLiteral("launcher-exit") << false;
        QTest::newRow("unresponsive-greeter-and-wm") << QStringLiteral("stubborn") << true;
    }

    void stopsCompositor()
    {
        QFETCH(QString, mode);
        QFETCH(bool, success);
        const QDir runtime(m_runtime->path());
        auto environment = QProcessEnvironment::systemEnvironment();
        environment.insert(QStringLiteral("XDG_RUNTIME_DIR"), runtime.path());
        m_helper.setProcessEnvironment(environment);
        const QString command = QStringLiteral("\"%1\"").arg(QStringLiteral(CLEANUP_PROCESS_PATH));
        m_helper.start(QStringLiteral(WAYLAND_HELPER_PATH), {
            command + QStringLiteral(" compositor ") + mode,
            command + QStringLiteral(" greeter ") + mode});
        QVERIFY(m_helper.waitForStarted());

        if (mode != QLatin1String("launcher-exit")) {
            const QString greeterPid = runtime.filePath(QStringLiteral("greeter.pid"));
            QTRY_VERIFY_WITH_TIMEOUT(QFile::exists(greeterPid) ||
                                     m_helper.state() == QProcess::NotRunning, 10000);
            QVERIFY2(QFile::exists(greeterPid), m_helper.readAllStandardError().constData());
            if (mode == QLatin1String("stubborn")) {
                m_helper.terminate();
            } else {
                QFile trigger(runtime.filePath(QStringLiteral("exit-greeter")));
                QVERIFY(trigger.open(QIODevice::WriteOnly));
            }
        }

        QTRY_COMPARE_WITH_TIMEOUT(m_helper.state(), QProcess::NotRunning, 25000);
        QCOMPARE(m_helper.exitStatus(), QProcess::NormalExit);
        QCOMPARE(m_helper.exitCode() == 0, success);
        QVERIFY(QFile::exists(runtime.filePath(QStringLiteral("compositor.pid"))));
        QVERIFY(QFile::exists(runtime.filePath(QStringLiteral("child.pid"))));
        // Verify the WM, greeter, and SIGTERM-resistant descendant are gone.
        for (const QString &file : runtime.entryList({QStringLiteral("*.pid")}, QDir::Files)) {
            const QString path = runtime.filePath(file);
            const pid_t pid = readPid(path);
            QVERIFY(pid > 0);
            QTRY_VERIFY_WITH_TIMEOUT(exited(pid), 3000);
            QVERIFY(QFile::remove(path));
        }
    }
};

QTEST_GUILESS_MAIN(WaylandCleanupTest)
#include "WaylandCleanupTest.moc"
