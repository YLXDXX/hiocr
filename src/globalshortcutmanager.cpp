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

// D-Bus 传输用的快捷键结构体
struct Shortcut {
    QString id;
    QVariantMap properties;
};

// 序列化操作符
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

GlobalShortcutManager* GlobalShortcutManager::instance()
{
    static GlobalShortcutManager* s_instance = nullptr;
    if (!s_instance) {
        s_instance = new GlobalShortcutManager();
    }
    return s_instance;
}

GlobalShortcutManager::GlobalShortcutManager(QObject* parent)
: QObject(parent), m_isActive(false)
{
    qDBusRegisterMetaType<QVariantMap>();
    qDBusRegisterMetaType<Shortcut>();
    qDBusRegisterMetaType<QList<Shortcut>>();
}

void GlobalShortcutManager::startListening() {
    static bool connectionEstablished = false;
    if (m_isActive || connectionEstablished) return;

    if (QGuiApplication::platformName() == "wayland") {
        connectionEstablished = true;
        tryAcquireSession();

        // 修复：连接信号时，必须确保签名匹配
        // Activated 信号签名: (o session_handle, s shortcut_id, t timestamp, a{sv} options)
        QDBusConnection::sessionBus().connect(
            "org.freedesktop.portal.Desktop",
            "/org/freedesktop/portal/desktop",
            "org.freedesktop.portal.GlobalShortcuts",
            "Activated",
            this,
            // 注意：这里参数类型必须与头文件中的槽函数完全一致
            SLOT(onActivated(QDBusObjectPath,QString,quint64,QVariantMap))
        );
    }
}

void GlobalShortcutManager::registerShortcut(const QString& id, const QString& description, const QString& preferredShortcut)
{
    m_currentId = id;
    m_currentDescription = description;
    m_currentPreferredShortcut = preferredShortcut;
}

void GlobalShortcutManager::tryAcquireSession()
{
    QDBusInterface portal("org.freedesktop.portal.Desktop",
                          "/org/freedesktop/portal/desktop",
                          "org.freedesktop.portal.GlobalShortcuts");

    if (!portal.isValid()) {
        qDebug() << "GlobalShortcuts portal not available";
        return;
    }

    QVariantMap options;
    options["session_handle_token"] = QString("session%1").arg(QRandomGenerator::global()->generate());

    QDBusPendingReply<QDBusObjectPath> reply = portal.asyncCall("CreateSession", options);

    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &GlobalShortcutManager::onSessionCreated);
}

void GlobalShortcutManager::onSessionCreated(QDBusPendingCallWatcher* watcher)
{
    watcher->deleteLater();
    QDBusPendingReply<QDBusObjectPath> reply = *watcher;
    if (reply.isError()) {
        qDebug() << "Failed to create session:" << reply.error().message();
        return;
    }

    QDBusObjectPath sessionPath = reply.value();

    // 监听 CreateSession 请求的 Response 信号
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
            callBindShortcuts();
        }
    } else {
        qDebug() << "Session request denied, code:" << code;
    }
}

void GlobalShortcutManager::callBindShortcuts() {
    QVariantMap props;
    props["description"] = m_currentDescription;
    if (!m_currentPreferredShortcut.isEmpty()) {
        props["preferred_trigger"] = m_currentPreferredShortcut;
    }

    Shortcut s = { m_currentId, props };

    QDBusArgument arg;
    arg.beginArray(qMetaTypeId<Shortcut>());
    arg << s;
    arg.endArray();

    QDBusMessage message = QDBusMessage::createMethodCall(
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.GlobalShortcuts",
        "BindShortcuts"
    );

    message << QVariant::fromValue(QDBusObjectPath(m_sessionHandle))
    << QVariant::fromValue(arg)
    << QString("") // parent_window
    << QVariantMap();

    QDBusPendingCall bindCall = QDBusConnection::sessionBus().asyncCall(message);
    QDBusPendingCallWatcher* bindWatcher = new QDBusPendingCallWatcher(bindCall, this);
    connect(bindWatcher, &QDBusPendingCallWatcher::finished, this, &GlobalShortcutManager::onShortcutsRegistered);
}

void GlobalShortcutManager::onShortcutsRegistered(QDBusPendingCallWatcher* watcher)
{
    watcher->deleteLater();
    QDBusPendingReply<QDBusObjectPath> reply = *watcher;

    if (reply.isError()) {
        qDebug() << "Failed to bind shortcuts:" << reply.error().message();
        return;
    }

    qDebug() << "Global shortcuts registered successfully. ID:" << m_currentId;
    m_isActive = true;
}

// 修复：实现正确的槽函数签名
void GlobalShortcutManager::onActivated(const QDBusObjectPath &session, const QString &shortcut_id, quint64 timestamp, const QVariantMap &options) {
    Q_UNUSED(timestamp)
    Q_UNUSED(options)

    // 调试信息
    qDebug() << "Shortcut activated! Session:" << session.path() << "ID:" << shortcut_id;

    if (session.path() == m_sessionHandle) {
        // 只发射信号，不直接操作截图
        emit activated(shortcut_id);
    }
}
