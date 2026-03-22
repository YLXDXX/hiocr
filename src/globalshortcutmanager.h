#ifndef GLOBALSHORTCUTMANAGER_H
#define GLOBALSHORTCUTMANAGER_H

#include <QObject>
#include <QDBusObjectPath>
#include <QVariantMap>
#include <QDBusPendingCallWatcher>

class GlobalShortcutManager : public QObject
{
    Q_OBJECT
public:
    static GlobalShortcutManager* instance();

    // 注册快捷键信息（描述和默认键位）
    void registerShortcut(const QString& id, const QString& description, const QString& preferredShortcut);

    // 开始监听（仅执行一次连接）
    void startListening();

signals:
    // 快捷键被触发时发射此信号
    void activated(const QString& id);

private slots:
    // 1. Session 创建流程
    void onSessionCreated(QDBusPendingCallWatcher* watcher);
    void onSessionResponse(uint code, const QVariantMap& results);
    void onShortcutsRegistered(QDBusPendingCallWatcher* watcher);

    // 2. 快捷键触发回调 (核心修复点)
    // 修正参数列表，增加 timestamp 以匹配 D-Bus 信号签名 (o, s, t, a{sv})
    void onActivated(const QDBusObjectPath &session, const QString &shortcut_id, quint64 timestamp, const QVariantMap &options);

private:
    explicit GlobalShortcutManager(QObject* parent = nullptr);

    // 内部辅助函数
    void tryAcquireSession();
    void callBindShortcuts();

    // 存储当前注册信息
    QString m_currentId;
    QString m_currentDescription;
    QString m_currentPreferredShortcut;

    // 存储 Session 路径
    QString m_sessionHandle;

    bool m_isActive;
};

#endif // GLOBALSHORTCUTMANAGER_H
