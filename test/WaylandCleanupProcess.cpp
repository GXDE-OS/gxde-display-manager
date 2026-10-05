#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QLocalServer>
#include <QSaveFile>
#include <QThread>

#include <signal.h>
#include <unistd.h>

static bool publishPid(const QString &path)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    const QByteArray pid = QByteArray::number(getpid());
    return file.write(pid) == pid.size() && file.commit();
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    const QStringList args = app.arguments();
    if (args.size() != 3)
        return 2;

    const QDir runtime(qEnvironmentVariable("XDG_RUNTIME_DIR"));
    const QString role = args.at(1);
    const QString mode = args.at(2);
    if (mode == QLatin1String("stubborn"))
        signal(SIGTERM, SIG_IGN);
    if (!publishPid(runtime.filePath(role + QStringLiteral(".pid"))))
        return 2;

    if (role == QLatin1String("compositor")) {
        const pid_t child = fork();
        if (child < 0)
            return 2;
        if (child == 0) {
            signal(SIGTERM, SIG_IGN);
            if (!publishPid(runtime.filePath(QStringLiteral("child.pid"))))
                _exit(2);
            for (;;)
                pause();
        }
        while (!QFileInfo::exists(runtime.filePath(QStringLiteral("child.pid"))))
            QThread::msleep(10);
        if (mode == QLatin1String("launcher-exit"))
            return 1;

        QLocalServer display;
        if (!display.listen(runtime.filePath(QStringLiteral("wayland-0"))))
            return 2;
        for (;;)
            pause();
    }

    while (!QFileInfo::exists(runtime.filePath(QStringLiteral("exit-greeter"))))
        QThread::msleep(10);
    return mode == QLatin1String("crash") ? 1 : 0;
}
