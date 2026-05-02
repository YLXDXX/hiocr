#ifndef KDEGLOBALSHORTCUT_H
#define KDEGLOBALSHORTCUT_H

#include <QObject>
#include <QMap>
#include <QAction>
#include <QDebug>
#include <KGlobalAccel>

class KdeGlobalShortcut : public QObject
{
    Q_OBJECT
public:
    static KdeGlobalShortcut* instance();

    void registerShortcut(const QString& id, const QString& description, const QString& defaultShortcut);
    void startListening();

signals:
    void activated(const QString& id);

private:
    explicit KdeGlobalShortcut(QObject* parent = nullptr);
    QMap<QString, QAction*> m_actions;
};

// --- Implementation ---

inline KdeGlobalShortcut* KdeGlobalShortcut::instance() {
    static KdeGlobalShortcut* s_instance = nullptr;
    if (!s_instance) s_instance = new KdeGlobalShortcut();
    return s_instance;
}

inline KdeGlobalShortcut::KdeGlobalShortcut(QObject* parent) : QObject(parent) {}

inline void KdeGlobalShortcut::registerShortcut(const QString& id, const QString& description, const QString& defaultShortcut) {
    QAction* action = m_actions.value(id, nullptr);

    if (!action) {
        action = new QAction(this);
        action->setObjectName(id);
        action->setText(description);

        connect(action, &QAction::triggered, this, [this, id]() {
            emit activated(id);
        });

        m_actions[id] = action;
    }

    QList<QKeySequence> shortcuts;
    shortcuts << QKeySequence(defaultShortcut);

    KGlobalAccel::self()->setShortcut(action, shortcuts, KGlobalAccel::NoAutoloading);
    KGlobalAccel::self()->setDefaultShortcut(action, shortcuts);

    qDebug() << "Updated global shortcut via KGlobalAccel:" << id << defaultShortcut;
}

inline void KdeGlobalShortcut::startListening() {
    qDebug() << "KGlobalAccel ready";
}

#endif // KDEGLOBALSHORTCUT_H
