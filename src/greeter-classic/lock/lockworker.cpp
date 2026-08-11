/*
 * Copyright (C) 2026 CharOfString <markus_verify@126.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * ----------------------------------------------------------------------------
 * Basically rewritten, for DDE Daemon is NO LONGER needed in GXDM. Hence, its
 * auth paths (e.g. DeepinAuthFramework, DBusLockService, etc.) are NO LONGER
 * available, so we reuse GXDM's own SDDM::Auth in "auth-only" mode: with no
 * session set, gxdm-helper runs the gxdm PAM stack and exits on success without
 * starting a session. No hand-rolled PAM, no direct logind.
 */
// PATCHS
#include "lockworker.h"
#include "sessionbasemodel.h"
#include "userinfo.h"

#include "AuthRequest.h"
#include "AuthPrompt.h"

#include <unistd.h>

#include <QDebug>
#include <QProcess>

using namespace Auth;  // deepin Auth namespace (AuthInterface); SDDM::Auth stays qualified

LockWorker::LockWorker(SessionBaseModel* const model, QObject* parent)
        : AuthInterface(model, parent), m_authenticating(false),
          m_auth(new SDDM::Auth(this)) {
    m_currentUserUid = getuid();

    // Reuse GXDM's authenticator. Forward the helper's password prompt(s) and
    // the final result.
    m_auth->setVerbose(true);
    connect(m_auth, &SDDM::Auth::requestChanged, this,
            &LockWorker::slotRequestChanged);
    connect(m_auth, &SDDM::Auth::authentication, this,
            &LockWorker::onAuthentication);

    const QString &switchUserButtonValue{valueByQSettings<QString>("Lock",
        "showSwitchUserButton", "ondemand")};
    m_model->setAlwaysShowUserSwitchButton(switchUserButtonValue == "always");
    m_model->setAllowShowUserSwitchButton(switchUserButtonValue == "ondemand");

    if (valueByQSettings<bool>("", "loginPromptAvatar", true)) {
        initDBus();
        initData();
    }
}

void LockWorker::switchToUser(std::shared_ptr<User> user) {
    if (!user)
        return;

    // clear old password
    m_password.clear();

    // Switching to another user means handing control back to the display
    // manager greeter. dde-switchtogreeter is best-effort (absent on non-DDE);
    // full fast-user-switch via GXDM is a later milestone.
    if (user->isLogin())
        QProcess::startDetached(QStringLiteral("dde-switchtogreeter"),
            {user->name()});
    else
        QProcess::startDetached(QStringLiteral("dde-switchtogreeter"), {});
}

void LockWorker::authUser(const QString& password) {
    if (m_authenticating)
        return;

    std::shared_ptr<User> user = m_model->currentUser();
    if (!user)
        return;

    m_authenticating = true;
    m_password = password;

    qDebug() << "(LockWorker) authenticating user via GXDM helper:"
             << user->name();

    // Auth-only: set the user but NOT a session. gxdm-helper authenticates
    // against the gxdm PAM stack and exits HELPER_SUCCESS without opening a
    // session (see HelperApp: the session block is skipped when no path is set).
    m_auth->setUser(user->name());
    m_auth->start();
}

void LockWorker::enableZoneDetected(bool disable) {
    // Hot-zone (com.deepin.daemon.Zone) removed; nothing to do.
    Q_UNUSED(disable)
}

void LockWorker::onUserAdded(const QString& user) {
    std::shared_ptr<User> user_ptr(new NativeUser(user));
    user_ptr->setisLogind(isLogined(user_ptr->uid()));

    // The locker only ever authenticates the session's own user.
    if (user_ptr->uid() == m_currentUserUid)
        m_model->setCurrentUser(user_ptr);

    if (user_ptr->uid() == m_lastLogoutUid)
        m_model->setLastLogoutUser(user_ptr);

    m_model->userAdd(user_ptr);
}

void LockWorker::slotRequestChanged() {
    // Feed the stored password into the helper's PAM prompt(s), mirroring the
    // daemon's Display::slotRequestChanged.
    SDDM::AuthRequest* req = m_auth->request();
    if (req->prompts().length() == 1) {
        req->prompts()[0]->setResponse(m_password.toUtf8());
        req->done();
    } else if (req->prompts().length() == 2) {
        req->prompts()[0]->setResponse(m_model->currentUser()->name().toUtf8());
        req->prompts()[1]->setResponse(m_password.toUtf8());
        req->done();
    }
}

void LockWorker::onAuthentication(const QString& user, bool ok) {
    Q_UNUSED(user)
    m_authenticating = false;

    emit m_model->authFinished(ok);

    if (!ok) {
        qDebug() << "(LockWorker) authentication failed";
        emit m_model->authFaildTipsMessage(tr("Wrong Password"));
        if (m_model->currentUser() && m_model->currentUser()->isLockForNum())
            m_model->currentUser()->startLock();
        return;
    }

    if (m_model->currentUser())
        m_model->currentUser()->resetLock();

    // NOTE: power actions from the lock (suspend / reboot / shutdown) are not
    // wired here on purpose -- they should be routed through GXDM rather than
    // hitting logind directly from the session. TODO.
}
// PATCHE
