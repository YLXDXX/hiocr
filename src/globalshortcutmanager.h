#ifndef GLOBALSHORTCUTMANAGER_H
#define GLOBALSHORTCUTMANAGER_H

#include <QObject>
#include <QDBusObjectPath>
#include <QVariantMap>
#include <QDBusPendingCallWatcher>
#include <QMap>

class GlobalShortcutManager : public QObject
{
    Q_OBJECT
public:
    static GlobalShortcutManager* instance();

    void registerShortcut(const QString& id, const QString& description, const QString& preferredShortcut);
    void unregisterShortcut(const QString& id);
    void startListening();

signals:
    void activated(const QString& id);
    void errorOccurred(const QString& message);

private slots:
    void onSessionCreated(QDBusPendingCallWatcher* watcher);
    void onSessionResponse(uint code, const QVariantMap& results);
    void onShortcutsRegistered(QDBusPendingCallWatcher* watcher);
    void onActivated(const QDBusObjectPath &session, const QString &shortcut_id, quint64 timestamp, const QVariantMap &options);

private:
    explicit GlobalShortcutManager(QObject* parent = nullptr);
    void tryAcquireSession();
    void bindAllShortcuts();

    QMap<QString, QVariantMap> m_shortcuts;
    QString m_sessionHandle;
    bool m_isActive;
};

#endif // GLOBALSHORTCUTMANAGER_H
