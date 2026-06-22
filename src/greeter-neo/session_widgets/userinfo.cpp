#include "userinfo.h"

#include <QFile>
#include <QUrl>
#include <QStringList>
#include <QTimer>

#include <unistd.h>
#include <pwd.h>
#include <grp.h>

#define LOCK_AUTH_NUM 5

static bool checkUserIsNoPWGrp(User const * user)
{
    if (user->type() == User::ADDomain) {
        return false;
    }

    // Caution: 32 here is unreliable, and there may be more
    // than this number of groups that the user joins.

    int ngroups = 32;
    gid_t groups[32];
    struct passwd pw;
    struct group gr;

    /* Fetch passwd structure (contains first group ID for user) */

    pw = *getpwnam(user->name().toUtf8().data());

    /* Retrieve group list */

    if (getgrouplist(user->name().toUtf8().data(), pw.pw_gid, groups, &ngroups) == -1) {
        fprintf(stderr, "getgrouplist() returned -1; ngroups = %d\n",
                ngroups);
        return false;
    }

    /* Display list of retrieved groups, along with group names */

    for (int i = 0; i < ngroups; i++) {
        gr = *getgrgid(groups[i]);
        if (QString(gr.gr_name) == QString("nopasswdlogin")) {
            return true;
        }
    }

    return false;
}

static const QString toLocalFile(const QString &path) {
    QUrl url(path);

    if (url.isLocalFile()) {
        return url.path();
    }

    return url.url();
}

User::User(QObject *parent)
    : QObject(parent)
    , m_isLogind(false)
    , m_isLock(false)
    , m_lockNum(4)
    , m_tryNum(5)
    , m_lockTimer(new QTimer)
{
    m_lockTimer->setInterval(1000 * 60);
    m_lockTimer->setSingleShot(false);
    connect(m_lockTimer.get(), &QTimer::timeout, this, &User::onLockTimeOut);
}

User::User(const User &user)
    : QObject(user.parent())
    , m_isLogind(user.m_isLogind)
    , m_isLock(user.m_isLock)
    , m_uid(user.m_uid)
    , m_lockNum(user.m_lockNum)
    , m_tryNum(user.m_tryNum)
    , m_userName(user.m_userName)
    , m_locale(user.m_locale)
    , m_lockTimer(user.m_lockTimer)
{

}

bool User::operator==(const User &user) const
{
    return type() == user.type() &&
            m_uid == user.m_uid;
}

void User::setLocale(const QString &locale)
{
    if (m_locale == locale) return;

    m_locale = locale;

    emit localeChanged(locale);
}

bool User::isNoPasswdGrp() const
{
    return checkUserIsNoPWGrp(this);
}

void User::setisLogind(bool isLogind) {
    if (m_isLogind == isLogind) {
        return;
    }

    m_isLogind = isLogind;

    emit logindChanged(isLogind);
}

void User::setPath(const QString &path)
{
    if (m_path == path) return;

    m_path = path;
}

bool User::isLockForNum()
{
    return m_isLock || --m_tryNum == 0;
}

void User::startLock()
{
    if (m_lockTimer->isActive()) return;

    m_isLock = true;

    onLockTimeOut();
}

void User::resetLock()
{
    m_tryNum = 5;
}

void User::onLockTimeOut()
{
    if (m_lockNum == 1) {
        m_lockTimer->stop();
        m_tryNum = 5;
        m_lockNum = 4;
        m_isLock = false;
    }
    else {
        m_lockNum--;
        m_lockTimer->start();
    }

    emit lockChanged(m_tryNum == 0);
}

NativeUser::NativeUser(const QString &userName, QObject *parent)
    : User(parent)
{
    // Source identity from the system password database (no dde-daemon).
    struct passwd *pw = getpwnam(userName.toUtf8().constData());
    if (pw) {
        m_userName = QString::fromLocal8Bit(pw->pw_name);
        m_uid = pw->pw_uid;
        const QString gecos = QString::fromLocal8Bit(pw->pw_gecos ? pw->pw_gecos : "");
        m_fullName = gecos.section(QLatin1Char(','), 0, 0);
    } else {
        m_userName = userName;
        m_uid = 0;
    }

    setPath(m_userName);
}

void NativeUser::setCurrentLayout(const QString &layout)
{
    if (m_currentLayout == layout)
        return;
    m_currentLayout = layout;
    emit currentKBLayoutChanged(layout);
}

QString NativeUser::displayName() const
{
    return m_fullName.isEmpty() ? name() : m_fullName;
}

QString NativeUser::avatarPath() const
{
    // Prefer the AccountsService icon, then ~/.face, then a bundled default.
    const QString svc = QStringLiteral("/var/lib/AccountsService/icons/%1").arg(m_userName);
    if (QFile::exists(svc))
        return svc;

    struct passwd *pw = getpwnam(m_userName.toUtf8().constData());
    if (pw && pw->pw_dir) {
        const QString face = QString::fromLocal8Bit(pw->pw_dir) + QStringLiteral("/.face");
        if (QFile::exists(face))
            return face;
    }

    return QStringLiteral(":/img/default_avatar.png");
}

QString NativeUser::greeterBackgroundPath() const
{
    return QStringLiteral("/usr/share/backgrounds/default_background.jpg");
}

QString NativeUser::desktopBackgroundPath() const
{
    return greeterBackgroundPath();
}

QStringList NativeUser::kbLayoutList()
{
    return QStringList();
}

QString NativeUser::currentKBLayout()
{
    return m_currentLayout;
}

bool NativeUser::isNoPasswdGrp() const
{
    return checkUserIsNoPWGrp(this);
}

ADDomainUser::ADDomainUser(uint uid, QObject *parent)
    : User(parent)
{
    m_uid = uid;
}

void ADDomainUser::setUserDisplayName(const QString &name)
{
    if (m_displayName == name) {
        return;
    }

    m_displayName = name;

    emit displayNameChanged(name);
}

void ADDomainUser::setUserName(const QString &name)
{
    if (m_userName == name) {
        return;
    }

    m_userName = name;
}

QString ADDomainUser::displayName() const
{
    return m_displayName.isEmpty() ? m_userName : m_displayName;
}

QString ADDomainUser::avatarPath() const
{
    return QString(":/img/default_avatar.png");
}

QString ADDomainUser::greeterBackgroundPath() const
{
    return QString("/usr/share/wallpapers/deepin/desktop.jpg");
}

QString ADDomainUser::desktopBackgroundPath() const
{
    return QString("/usr/share/wallpapers/deepin/desktop.jpg");
}
