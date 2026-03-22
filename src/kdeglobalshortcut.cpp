#include "kdeglobalshortcut.h"
#include <KGlobalAccel>
#include <QAction>
#include <QDebug>

KdeGlobalShortcut* KdeGlobalShortcut::instance() {
    static KdeGlobalShortcut* s_instance = nullptr;
    if (!s_instance) s_instance = new KdeGlobalShortcut();
    return s_instance;
}

KdeGlobalShortcut::KdeGlobalShortcut(QObject* parent) : QObject(parent) {}

void KdeGlobalShortcut::registerShortcut(const QString& id, const QString& description, const QString& defaultShortcut) {
    if (m_actions.contains(id)) return;

    QAction* action = new QAction(this);
    action->setObjectName(id);
    action->setText(description);

    QList<QKeySequence> shortcuts;
    shortcuts << QKeySequence(defaultShortcut);
    KGlobalAccel::self()->setDefaultShortcut(action, shortcuts);
    KGlobalAccel::self()->setShortcut(action, shortcuts);

    connect(action, &QAction::triggered, this, [this, id]() {
        emit activated(id);
    });

    m_actions[id] = action;
    qDebug() << "Registered global shortcut via KGlobalAccel:" << id << defaultShortcut;
}

void KdeGlobalShortcut::startListening() {
    qDebug() << "KGlobalAccel ready";
}
