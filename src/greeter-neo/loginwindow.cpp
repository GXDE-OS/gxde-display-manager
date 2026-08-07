/*
 * Copyright (C) 2011 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     sbw <sbw@sbw.so>
 *             kirigaya <kirigaya@mkacg.com>
 *             Hualet <mr.asianwang@gmail.com>
 *
 * Maintainer: sbw <sbw@sbw.so>
 *             kirigaya <kirigaya@mkacg.com>
 *             Hualet <mr.asianwang@gmail.com>
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
 */

#include "loginwindow.h"
#include "lockcontent.h"
#include "sessionindicator.h"
#include "view/logowidget.h"
#include "userinfo.h"

LoginWindow::LoginWindow(SessionBaseModel * const model, QWidget *parent)
    : FullscreenBackground(parent)
    , m_loginFrame( new LockContent(model, this))
    , m_inactiveSessionIndicator(new SessionIndicator(this))
{
    LogoWidget *logoWidget = new LogoWidget;
    m_loginFrame->setLeftBottomWidget(logoWidget);
    setContent(m_loginFrame);
    setBackgroundFocusedImmediately(false);
    m_loginFrame->hide();

    m_inactiveSessionIndicator->move(
        width() - m_inactiveSessionIndicator->width() - 60,
        height() - m_inactiveSessionIndicator->height() - 33);
    m_inactiveSessionIndicator->setSession(model->sessionKey());
    m_inactiveSessionIndicator->setVisible(m_loginFrame->sessionSwitcherEnabled());
    m_inactiveSessionIndicator->raise();

    connect(this, &FullscreenBackground::contentVisibleChanged,
        this, [this](bool visible) {
            m_inactiveSessionIndicator->setVisible(
                !visible && m_loginFrame->sessionSwitcherEnabled());
            if (!visible)
                m_inactiveSessionIndicator->raise();
        });
    connect(model, &SessionBaseModel::onSessionKeyChanged,
        m_inactiveSessionIndicator, &SessionIndicator::setSession);
    connect(m_inactiveSessionIndicator, &SessionIndicator::clicked,
        this, [this, model] {
            setContentVisible(true);
            model->setCurrentModeState(SessionBaseModel::ModeStatus::SessionMode);
        });

    connect(m_loginFrame, &LockContent::requestBackground, this, [=] (const QString &wallpaper) {
        updateBackground(wallpaper);
#ifdef DISABLE_LOGIN_ANI
        // 在认证成功以后会通过更改背景来实现登录动画，但是禁用登录动画的情况下，会立即调用startSession，
        // 导致当前进程被lightdm退掉，X上会残留上一帧的画面，可以看到输入框等画面。使用repaint()强制刷新背景来避免这个问题。
        repaint();
#endif
    });
    connect(m_loginFrame, &LockContent::requestBackgroundFocus,
        this, &FullscreenBackground::setBackgroundFocused);

    connect(model, &SessionBaseModel::authFinished, this, [=] (bool successd) {
        m_loginFrame->setVisible(!successd);
        if (successd)
            m_inactiveSessionIndicator->hide();
#ifdef DISABLE_LOGIN_ANI
        // 在认证成功以后会通过更改背景来实现登录动画，但是禁用登录动画的情况下，会立即调用startSession，
        // 导致当前进程被lightdm退掉，X上会残留上一帧的画面，可以看到输入框等画面。使用repaint()强制刷新背景来避免这个问题。
        repaint();
#endif
    });

    connect(m_loginFrame, &LockContent::requestAuthUser, this, &LoginWindow::requestAuthUser);
    connect(m_loginFrame, &LockContent::requestSwitchToUser, this, &LoginWindow::requestSwitchToUser);
    connect(m_loginFrame, &LockContent::requestSetLayout, this, &LoginWindow::requestSetLayout);

    connect(model, &SessionBaseModel::currentUserChanged, this, [=] (std::shared_ptr<User> user) {
        if (user.get()) {
            logoWidget->updateLocale(user->locale().split(".").first());
        }
    });
}

void LoginWindow::resizeEvent(QResizeEvent *event)
{
    FullscreenBackground::resizeEvent(event);
    m_inactiveSessionIndicator->move(
        width() - m_inactiveSessionIndicator->width() - 60,
        height() - m_inactiveSessionIndicator->height() - 33);
    if (m_inactiveSessionIndicator->isVisible())
        m_inactiveSessionIndicator->raise();
}
