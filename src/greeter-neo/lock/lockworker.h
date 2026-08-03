#ifndef LOCKWORKER_H
#define LOCKWORKER_H
// PATCHS
// Substantially rewritten vs the dde-lock original. dde-daemon auth (Authority /
// DBusLockService / Hotzone / Accounts / Logined) is gone. Unlock reuses GXDM's
// own SDDM::Auth in "auth-only" mode: with no session set, gxdm-helper runs the
// gxdm PAM stack and exits on success without starting a session. No hand-rolled
// PAM and no direct logind here.

#include "userinfo.h"
#include "authinterface.h"

#include "Auth.h"  // SDDM::Auth (src/auth)

#include <QObject>

class SessionBaseModel;
class LockWorker : public Auth::AuthInterface
{
    Q_OBJECT
public:
    explicit LockWorker(SessionBaseModel *const model, QObject *parent = nullptr);

    void switchToUser(std::shared_ptr<User> user) override;
    void authUser(const QString &password) override;

    void enableZoneDetected(bool disable);  // hot-zone removed; kept as no-op

private:
    void onUserAdded(const QString &user) override;
    void slotRequestChanged();                          // feed the password to PAM
    void onAuthentication(const QString &user, bool ok);  // unlock result

    bool m_authenticating;
    SDDM::Auth *m_auth;
    QString m_password;
};

// PATCHE
#endif  // LOCKWORKER_H
