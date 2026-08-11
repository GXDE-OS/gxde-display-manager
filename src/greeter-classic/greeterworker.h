#ifndef GREETERWORKER_H
#define GREETERWORKER_H

#include <QObject>

#include "authinterface.h"
#include "sessionbasemodel.h"

class QLocalSocket;

class GreeterWorker : public Auth::AuthInterface {
    Q_OBJECT
public:
    explicit GreeterWorker(SessionBaseModel *const model, const QString &socket,
                           QObject *parent = nullptr);

    void switchToUser(std::shared_ptr<User> user) override;
    void authUser(const QString &password) override;

signals:
    void requestUpdateBackground(const QString &path);

private:
    void onConnected();
    void onReadyRead();
    void sendPowerAction(SessionBaseModel::PowerAction action);

    QLocalSocket *m_socket;
    bool m_authenticating = false;
};

#endif  // GREETERWORKER_H
