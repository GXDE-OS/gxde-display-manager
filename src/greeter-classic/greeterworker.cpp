#include "greeterworker.h"
#include "greeterappearance.h"
#include "sessionbasemodel.h"
#include "systemdefaults.h"
#include "userinfo.h"

#include "src/backend/sessions/sessions.h"

#include <QLocalSocket>
#include <QDataStream>
#include <QDebug>
#include <QFileInfo>

namespace {
// Mirrors SDDM::GreeterMessages / DaemonMessages (src/common/Messages.h)
// 参考SDDM::GreeterMessages / DaemonMessages (src/common/Messages.h)
enum class GreeterMessages : quint32 {
    Connect = 0,
    Login,
    PowerOff,
    Reboot,
    Suspend,
    Hibernate,
    HybridSleep,
};

enum class DaemonMessages : quint32 {
    HostName = 0,
    Capabilities,
    LoginSucceeded,
    LoginFailed,
    InformationMessage,
    LastSession,
};

// SDDM::Session::Type
enum SddmSessionType : quint32 {
    UnknownSession = 0,
    X11Session,
    WaylandSession,
};

// SDDM::Capability
constexpr quint32 kCapSuspend = 0x0004;

QString sessionKeyForDesktopFile(const QString& desktopFile) {
    if (desktopFile.isEmpty()) {
        return QString();
    }

    const QString desktopName = QFileInfo(desktopFile).completeBaseName();
    gxdm::backend::Sessions sessions;
    const auto table = sessions.GetSessionHashTable();
    for (const auto &entry : table) {
        const QString candidate = QString::fromStdString(
            entry.second.xdg_session_desktop);
        if (candidate.compare(desktopName, Qt::CaseInsensitive) == 0) {
            return QString::fromStdString(entry.first);
        }
    }

    return QString();
}
}  // namespace

GreeterWorker::GreeterWorker(SessionBaseModel *const model, const QString &socket, QObject *parent)
    : AuthInterface(model, parent)
    , m_socket(new QLocalSocket(this))
{
    connect(m_socket, &QLocalSocket::connected, this, &GreeterWorker::onConnected);
    connect(m_socket, &QLocalSocket::readyRead, this, &GreeterWorker::onReadyRead);
    connect(m_socket, &QLocalSocket::errorOccurred, this, [this] {
        qWarning() << "gxdm greeter: socket error:" << m_socket->errorString();
    });

    // The greeter runs as the unprivileged gxdm user, cannot drive logind directly.
    // PWR actions forwarded to daemon.
    // Greeter在用户层没发直接用logind，让daemon处理
    connect(model, &SessionBaseModel::onPowerActionChanged, this, &GreeterWorker::sendPowerAction);

    initDBus();
    initData();

    if (!socket.isEmpty()) {
        m_socket->connectToServer(socket);
    } else {
        qWarning() << "GreeterWorker: no --socket given; login backend disabled.";
    }
}

void GreeterWorker::onConnected()
{
    QByteArray data;
    QDataStream w(&data, QIODevice::WriteOnly);
    w << quint32(GreeterMessages::Connect);
    m_socket->write(data);
    m_socket->flush();
}

void GreeterWorker::switchToUser(std::shared_ptr<User> user)
{
    if (!user)
        return;

    m_model->setCurrentUser(user);

    // 登录界面壁纸：显式配置了全局壁纸或单用户时使用 greeterBackgroundPath()
    // （单用户与其锁屏壁纸一致）；多用户且未配置时使用默认登录壁纸。
    // 与 LockContent::currentBackgroundPath 的逻辑保持一致。
    if (GxdmGreeterAppearance::hasConfiguredWallpaper()
        || m_model->userList().size() == 1) {
        emit requestUpdateBackground(user->greeterBackgroundPath());
    } else {
        emit requestUpdateBackground(GxdmGreeterAppearance::wallpaper());
    }
}

void GreeterWorker::authUser(const QString &password)
{
    if (m_authenticating)
        return;

    std::shared_ptr<User> user = m_model->currentUser();
    if (!user)
        return;

    m_authenticating = true;

    // session key -> SDDM Session (type, "<file>.desktop").
    quint32 sessionType = X11Session;
    QString sessionFile;
    const QString key = m_model->sessionKey();
    if (!key.isEmpty()) {
        gxdm::backend::Sessions sessions;
        const auto table = sessions.GetSessionHashTable();
        const auto it = table.find(key.toStdString());
        if (it != table.end()) {
            sessionType = (it->second.session_type == gxdm::backend::SessionType::kWayland)
                              ? WaylandSession
                              : X11Session;
            sessionFile = QString::fromStdString(it->second.xdg_session_desktop) + ".desktop";
        }
    }

    qDebug() << "(Greeter frontend) Login: for" << user->name() << ", session" << sessionType << sessionFile;

    QByteArray data;
    QDataStream w(&data, QIODevice::WriteOnly);
    w << quint32(GreeterMessages::Login) << user->name() << password
      << sessionType << sessionFile;
    m_socket->write(data);
    m_socket->flush();
}

void GreeterWorker::sendPowerAction(SessionBaseModel::PowerAction action)
{
    GreeterMessages msg;
    switch (action) {
    case SessionBaseModel::PowerAction::RequireShutdown:
        msg = GreeterMessages::PowerOff;
        break;
    case SessionBaseModel::PowerAction::RequireRestart:
        msg = GreeterMessages::Reboot;
        break;
    case SessionBaseModel::PowerAction::RequireSuspend:
        msg = GreeterMessages::Suspend;
        break;
    case SessionBaseModel::PowerAction::RequireHibernate:
        msg = GreeterMessages::Hibernate;
        break;
    default:
        return;
    }

    QByteArray data;
    QDataStream w(&data, QIODevice::WriteOnly);
    w << quint32(msg);
    m_socket->write(data);
    m_socket->flush();
}

void GreeterWorker::onReadyRead()
{
    QDataStream in(m_socket);

    while (m_socket->bytesAvailable() > 0) {
        quint32 message;
        in >> message;

        switch (DaemonMessages(message)) {
        case DaemonMessages::Capabilities: {
            quint32 caps;
            in >> caps;
            m_model->setCanSleep(caps & kCapSuspend);
            break;
        }
        case DaemonMessages::HostName: {
            QString hostName;
            in >> hostName;
            break;
        }
        case DaemonMessages::LoginSucceeded: {
            // The daemon now starts the session and tears down the greeter.
            // Daemon拉起session，关闭greeter
            emit m_model->authFinished(true);
            break;
        }
        case DaemonMessages::LoginFailed: {
            m_authenticating = false;
            emit m_model->authFaildTipsMessage(tr("Wrong Password"));
            break;
        }
        case DaemonMessages::InformationMessage: {
            QString msg;
            in >> msg;
            emit m_model->authFaildMessage(msg);
            break;
        }
        case DaemonMessages::LastSession: {
            QString desktopFile;
            in >> desktopFile;

            const QString initialSession =
                GxdmSystemDefaults::initialSessionDesktopFile(desktopFile);
            const bool usingSystemDefault =
                desktopFile.isEmpty() && !initialSession.isEmpty();
            desktopFile = initialSession;

            const QString sessionKey = sessionKeyForDesktopFile(desktopFile);
            if (!sessionKey.isEmpty()) {
                if (usingSystemDefault) {
                    qDebug()
                        << "(Frontend) No remembered session; Defaulting to"
                        << desktopFile << sessionKey;
                } else {
                    qDebug() << "(Frontend) Restoring last session:"
                        << desktopFile << sessionKey;
                }
                m_model->setSessionKey(sessionKey);
            } else if (usingSystemDefault) {
                qWarning() << "(Frontend) GXDE default session is unavailable:"
                    << desktopFile;
            } else if (!desktopFile.isEmpty()) {
                qWarning()
                    << "(Frontend) Remembered session is no longer available:"
                    << desktopFile;
            }
            break;
        }
        default:
            qWarning() << "(Frontend) Login: unknown daemon message" << message;
            return;
        }
    }
}
