#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusObjectPath>
#include <QDBusReply>
#include <QDBusServiceWatcher>
#include <QDBusUnixFileDescriptor>
#include <QDebug>
#include <QFile>
#include <QImageReader>
#include <QKeyCombination>
#include <QTimer>
#include <QUrl>

#include "sessionbasemodel.h"
#include "lockshortcutmanager.h"

namespace {

const QString kKGlobalAccelService = QStringLiteral("org.kde.kglobalaccel");
const QString kKGlobalAccelPath = QStringLiteral("/kglobalaccel");
const QString kKGlobalAccelInterface = QStringLiteral("org.kde.KGlobalAccel");
const QString kKGlobalAccelComponentInterface = QStringLiteral(
  "org.kde.kglobalaccel.Component");
const QString kComponent = QStringLiteral("gxdm-lock-neo");
const QString kAction = QStringLiteral("lock-screen");
const QString kSystemDisplayManagerService =
  QStringLiteral("top.gxde.DisplayManager");
const QString kSystemDisplayManagerPath =
  QStringLiteral("/top/gxde/DisplayManager");
const QString kSystemDisplayManagerInterface =
  QStringLiteral("top.gxde.DisplayManager.System");

constexpr uint kSetPresent = 2;
constexpr uint kNoAutoloading = 4;

bool callSystemDisplayManager(const QString& method,
    const QVariantList& arguments = {}) {
  QDBusInterface interface(kSystemDisplayManagerService,
    kSystemDisplayManagerPath, kSystemDisplayManagerInterface,
    QDBusConnection::systemBus());
  const QDBusReply<bool> reply(interface.callWithArgumentList(
    QDBus::Block, method, arguments));
  if (!reply.isValid()) {
    qWarning() << method << "failed:" << reply.error().message();
    return false;
  }
  return reply.value();
}

QString querySystemDisplayManager(const QString& method) {
  QDBusInterface interface(kSystemDisplayManagerService,
    kSystemDisplayManagerPath, kSystemDisplayManagerInterface,
    QDBusConnection::systemBus());
  const QDBusReply<QString> reply(interface.call(QDBus::Block, method));
  if (!reply.isValid()) {
    return QString();
  }
  return reply.value();
}

}  // namespace

QDBusArgument& operator<<(
    QDBusArgument& argument, const DBusKeySequence& sequence) {
  argument.beginStructure();
  argument << sequence.keys;
  argument.endStructure();
  return argument;
}

const QDBusArgument& operator>>(
    const QDBusArgument& argument, DBusKeySequence& sequence) {
  argument.beginStructure();
  argument >> sequence.keys;
  argument.endStructure();
  return argument;
}

LockShortcutManager::LockShortcutManager(
    SessionBaseModel* model, QObject* parent)
    : QObject(parent),
      m_model(model),
      m_serviceWatcher(new QDBusServiceWatcher(kKGlobalAccelService,
          QDBusConnection::sessionBus(),
          QDBusServiceWatcher::WatchForOwnerChange, this)) {
  qDBusRegisterMetaType<DBusKeySequence>();
  qDBusRegisterMetaType<DBusKeySequenceList>();

  connect(m_serviceWatcher, &QDBusServiceWatcher::serviceOwnerChanged, this,
    &LockShortcutManager::onKGlobalAccelOwnerChanged);

  QDBusConnection::sessionBus().connect(kKGlobalAccelService, kKGlobalAccelPath,
    kKGlobalAccelInterface, QStringLiteral("yourShortcutsChanged"), this,
    SLOT(onShortcutsChanged(QStringList, DBusKeySequenceList)));
}

LockShortcutManager::~LockShortcutManager() {
  if (m_registered) withdraw(false);
}

bool LockShortcutManager::tryEnroll(bool enabled) {
  m_enrollmentRequested = enabled;
  if (!enabled) {
    withdraw(true);
    return false;
  }

  return enroll();
}

bool LockShortcutManager::isRegistered() const { return m_registered; }

void LockShortcutManager::showLock() {
  if (m_model) m_model->setIsShow(true);
}

void LockShortcutManager::onGlobalShortcutPressed(
    const QString& component, const QString& action, qlonglong timestamp) {
  Q_UNUSED(timestamp)
  if (component == kComponent && action == kAction) showLock();
}

void LockShortcutManager::onShortcutsChanged(
    const QStringList& changedActionId, const DBusKeySequenceList& keys) {
  if (changedActionId == actionId()) setRegistered(containsSuperL(keys));
}

void LockShortcutManager::onKGlobalAccelOwnerChanged(
    const QString& service, const QString& oldOwner, const QString& newOwner) {
  Q_UNUSED(service)
  Q_UNUSED(oldOwner)

  m_componentPath.clear();
  setRegistered(false);
  if (m_enrollmentRequested && !newOwner.isEmpty()) {
    QTimer::singleShot(0, this, [this] { enroll(); });
  }
}

bool LockShortcutManager::enroll() {
  QDBusInterface interface(kKGlobalAccelService, kKGlobalAccelPath,
      kKGlobalAccelInterface, QDBusConnection::sessionBus());

  const QDBusMessage registerReply =
    interface.call(QStringLiteral("doRegister"), actionId());
  if (registerReply.type() == QDBusMessage::ErrorMessage) {
    qWarning() << "KGlobalAccel registration failed:"
      << registerReply.errorMessage();
    setRegistered(false);
    return false;
  }

  const DBusKeySequenceList requested{superLSequence()};
  const QDBusReply<DBusKeySequenceList> shortcutReply =
      interface.call(QStringLiteral("setShortcutKeys"), actionId(),
          QVariant::fromValue(requested), kSetPresent | kNoAutoloading);
  if (!shortcutReply.isValid()) {
    qWarning() << "Setting Super+L failed:"
      << shortcutReply.error().message();
    withdraw(true);
    return false;
  }

  const bool registered = containsSuperL(shortcutReply.value());
  if (!registered) {
    qInfo() << "Super+L is already registered;"
      " leaving the existing shortcut untouched.";
    withdraw(true);
    return false;
  }

  if (!connectComponentSignal()) {
    qWarning() << "Failed to subscribe to activations.";
    withdraw(true);
    return false;
  }

  setRegistered(true);
  qInfo() << "Registered Super+L.";
  return true;
}

void LockShortcutManager::withdraw(bool forget) {
  QDBusInterface interface(kKGlobalAccelService, kKGlobalAccelPath,
    kKGlobalAccelInterface, QDBusConnection::sessionBus());

  if (forget) {
    interface.call(QStringLiteral("unregister"), kComponent, kAction);
  } else {
    interface.call(QStringLiteral("setInactive"), actionId());
  }

  if (!m_componentPath.isEmpty()) {
    QDBusConnection::sessionBus().disconnect(kKGlobalAccelService,
      m_componentPath, kKGlobalAccelComponentInterface,
      QStringLiteral("globalShortcutPressed"), this,
      SLOT(onGlobalShortcutPressed(QString, QString, qlonglong)));
    m_componentPath.clear();
  }
  setRegistered(false);
}

bool LockShortcutManager::connectComponentSignal() {
  QDBusInterface interface(kKGlobalAccelService, kKGlobalAccelPath,
    kKGlobalAccelInterface, QDBusConnection::sessionBus());
  const QDBusReply<QDBusObjectPath> componentReply =
    interface.call(QStringLiteral("getComponent"), kComponent);
  if (!componentReply.isValid()) return false;

  m_componentPath = componentReply.value().path();
  return QDBusConnection::sessionBus().connect(kKGlobalAccelService,
    m_componentPath, kKGlobalAccelComponentInterface,
    QStringLiteral("globalShortcutPressed"), this,
    SLOT(onGlobalShortcutPressed(QString, QString, qlonglong)));
}

void LockShortcutManager::setRegistered(bool registered) {
  if (m_registered == registered) return;

  m_registered = registered;
  emit registrationChanged(registered);
}

QStringList LockShortcutManager::actionId() {
  return {
    kComponent,
    kAction,
    QStringLiteral("GXDM"),
    QStringLiteral("Lock Screen"),
  };
}

DBusKeySequence LockShortcutManager::superLSequence() {
  DBusKeySequence sequence;
  sequence.keys = {
    QKeyCombination(Qt::MetaModifier, Qt::Key_L).toCombined(),
    0,
    0,
    0,
  };
  return sequence;
}

bool LockShortcutManager::containsSuperL(const DBusKeySequenceList& sequences) {
  const int superL = superLSequence().keys.constFirst();
  for (const DBusKeySequence& sequence : sequences) {
    if (!sequence.keys.isEmpty() && sequence.keys.constFirst() == superL)
      return true;
  }
  return false;
}

GxdeDisplayManagerService::GxdeDisplayManagerService(
    LockShortcutManager* manager)
    : QDBusAbstractAdaptor(manager), m_manager(manager) {
  connect(manager, &LockShortcutManager::registrationChanged, this,
    &GxdeDisplayManagerService::LkScrStatChanged);
}

bool GxdeDisplayManagerService::TryEnrollLkScr(bool enabled) {
  return m_manager->tryEnroll(enabled);
}

bool GxdeDisplayManagerService::LkScrStat() const {
  return m_manager->isRegistered();
}

bool GxdeDisplayManagerService::SetCursor(const QString& theme) {
  return callSystemDisplayManager(QStringLiteral("SetCursor"), {theme});
}

bool GxdeDisplayManagerService::SetWallpaperGXDEDefault() {
  return callSystemDisplayManager(QStringLiteral("SetWallpaperGXDEDefault"));
}

bool GxdeDisplayManagerService::SetWallpaperDDELockDefault() {
  return callSystemDisplayManager(
    QStringLiteral("SetWallpaperDDELockDefault"));
}

bool GxdeDisplayManagerService::SetWallpaper(const QString& wallpaper) {
  const QUrl url(wallpaper);
  const QString localPath = url.isLocalFile() ? url.toLocalFile() : wallpaper;
  QImageReader reader(localPath);
  if (!reader.canRead()) return false;

  QFile file(localPath);
  if (!file.open(QIODevice::ReadOnly)) return false;

  const QDBusUnixFileDescriptor descriptor(file.handle());
  if (!descriptor.isValid()) return false;

  return callSystemDisplayManager(QStringLiteral("SetWallpaper"),
    {QVariant::fromValue(descriptor)});
}

bool GxdeDisplayManagerService::SetGreeterDisplayServer(
    const QString& displayServer) {
  return callSystemDisplayManager(QStringLiteral("SetGreeterDisplayServer"),
    {displayServer});
}

QString GxdeDisplayManagerService::GreeterDisplayServer() const {
  return querySystemDisplayManager(QStringLiteral("GreeterDisplayServer"));
}

void GxdeDisplayManagerService::Show() { m_manager->showLock(); }
