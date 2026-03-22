#include "globalshortcutmanager.h"
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusMessage>
#include <QDebug>
#include <QGuiApplication>
#include <QVariantMap>
#include <QRandomGenerator>
#include <QDBusArgument>
#include <QDBusMetaType>

struct Shortcut {
    QString id;
    QVariantMap properties;
};

QDBusArgument &operator<<(QDBusArgument &argument, const Shortcut &shortcut) {
    argument.beginStructure();
    argument << shortcut.id << shortcut.properties;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, Shortcut &shortcut) {
    argument.beginStructure();
    argument >> shortcut.id >> shortcut.properties;
    argument.endStructure();
    return argument;
}

GlobalShortcutManager* GlobalShortcutManager::instance() {
    static GlobalShortcutManager* s_instance = nullptr;
    if (!s_instance) s_instance = new GlobalShortcutManager();
    return s_instance;
}

GlobalShortcutManager::GlobalShortcutManager(QObject* parent)
: QObject(parent), m_isActive(false) {
    qDBusRegisterMetaType<QVariantMap>();
    qDBusRegisterMetaType<Shortcut>();
    qDBusRegisterMetaType<QList<Shortcut>>();
}

void GlobalShortcutManager::registerShortcut(const QString& id, const QString& description, const QString& preferredShortcut) {
    QVariantMap props;
    props["description"] = description;
    if (!preferredShortcut.isEmpty())
        props["preferred_trigger"] = preferredShortcut;
    m_shortcuts[id] = props;

    if (!m_sessionHandle.isEmpty())
        bindAllShortcuts();
}

void GlobalShortcutManager::unregisterShortcut(const QString& id) {
    m_shortcuts.remove(id);
    if (!m_sessionHandle.isEmpty())
        bindAllShortcuts();
}

void GlobalShortcutManager::startListening() {
    static bool connectionEstablished = false;
    if (m_isActive || connectionEstablished) return;

    if (QGuiApplication::platformName() == "wayland") {
        connectionEstablished = true;
        tryAcquireSession();

        QDBusConnection::sessionBus().connect(
            "org.freedesktop.portal.Desktop",
            "/org/freedesktop/portal/desktop",
            "org.freedesktop.portal.GlobalShortcuts",
            "Activated",
            this,
            SLOT(onActivated(QDBusObjectPath,QString,quint64,QVariantMap))
        );
    }
}

void GlobalShortcutManager::tryAcquireSession() {
    QDBusInterface portal("org.freedesktop.portal.Desktop",
                          "/org/freedesktop/portal/desktop",
                          "org.freedesktop.portal.GlobalShortcuts");
    if (!portal.isValid()) {
        qDebug() << "GlobalShortcuts portal not available";
        emit errorOccurred("GlobalShortcuts portal not available");
        return;
    }

    QVariantMap options;
    options["session_handle_token"] = QString("session%1").arg(QRandomGenerator::global()->generate());

    QDBusPendingReply<QDBusObjectPath> reply = portal.asyncCall("CreateSession", options);
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &GlobalShortcutManager::onSessionCreated);
}

void GlobalShortcutManager::onSessionCreated(QDBusPendingCallWatcher* watcher) {
    watcher->deleteLater();
    QDBusPendingReply<QDBusObjectPath> reply = *watcher;
    if (reply.isError()) {
        qDebug() << "Failed to create session:" << reply.error().message();
        emit errorOccurred("Failed to create session: " + reply.error().message());
        return;
    }

    QDBusObjectPath sessionPath = reply.value();
    QDBusConnection::sessionBus().connect(
        "org.freedesktop.portal.Desktop",
        sessionPath.path(),
                                          "org.freedesktop.portal.Request",
                                          "Response",
                                          this,
                                          SLOT(onSessionResponse(uint,QVariantMap))
    );
}

void GlobalShortcutManager::onSessionResponse(uint code, const QVariantMap& results) {
    if (code == 0) {
        if (results.contains("session_handle")) {
            m_sessionHandle = results.value("session_handle").toString();
            qDebug() << "Session acquired:" << m_sessionHandle;
            bindAllShortcuts();
        } else {
            qDebug() << "Session response missing session_handle";
        }
    } else {
        qDebug() << "Session request denied, code:" << code;
        emit errorOccurred("Session request denied");
    }
}

void GlobalShortcutManager::bindAllShortcuts() {
    if (m_sessionHandle.isEmpty()) return;

    QList<Shortcut> shortcuts;
    for (auto it = m_shortcuts.begin(); it != m_shortcuts.end(); ++it)
        shortcuts.append({it.key(), it.value()});

    QDBusArgument arg;
    arg.beginArray(qMetaTypeId<Shortcut>());
    for (const auto& s : shortcuts) arg << s;
    arg.endArray();

    QDBusMessage message = QDBusMessage::createMethodCall(
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.GlobalShortcuts",
        "BindShortcuts"
    );
    message << QVariant::fromValue(QDBusObjectPath(m_sessionHandle))
    << QVariant::fromValue(arg)
    << QString("")
    << QVariantMap();

    QDBusPendingCall bindCall = QDBusConnection::sessionBus().asyncCall(message);
    QDBusPendingCallWatcher* bindWatcher = new QDBusPendingCallWatcher(bindCall, this);
    connect(bindWatcher, &QDBusPendingCallWatcher::finished, this, &GlobalShortcutManager::onShortcutsRegistered);
}

void GlobalShortcutManager::onShortcutsRegistered(QDBusPendingCallWatcher* watcher) {
    watcher->deleteLater();
    QDBusPendingReply<QDBusObjectPath> reply = *watcher;
    if (reply.isError()) {
        qDebug() << "Failed to bind shortcuts:" << reply.error().message();
        emit errorOccurred("Failed to bind shortcuts: " + reply.error().message());
        return;
    }
    qDebug() << "All global shortcuts registered successfully. Count:" << m_shortcuts.size();
    m_isActive = true;
}

void GlobalShortcutManager::onActivated(const QDBusObjectPath &session, const QString &shortcut_id, quint64 timestamp, const QVariantMap &options) {
    Q_UNUSED(timestamp);
    Q_UNUSED(options);
    if (session.path() == m_sessionHandle)
        emit activated(shortcut_id);
}
