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
    QAction* action = m_actions.value(id, nullptr);

    // 【修改】如果 Action 不存在，则创建；如果存在，则复用并进行更新
    if (!action) {
        action = new QAction(this);
        action->setObjectName(id);
        action->setText(description);

        // 首次创建时连接信号
        connect(action, &QAction::triggered, this, [this, id]() {
            emit activated(id);
        });

        m_actions[id] = action;
    }

    // 【修改】无论 Action 是新建还是已存在，都强制设置新的快捷键
    // 这将更新 KDE 系统设置中的快捷键绑定，覆盖旧的配置
    QList<QKeySequence> shortcuts;
    shortcuts << QKeySequence(defaultShortcut);

    // setShortcut 会强制更新用户配置，setDefaultShortcut 只更新默认值
    KGlobalAccel::self()->setShortcut(action, shortcuts, KGlobalAccel::NoAutoloading);
    KGlobalAccel::self()->setDefaultShortcut(action, shortcuts);

    qDebug() << "Updated global shortcut via KGlobalAccel:" << id << defaultShortcut;
}

void KdeGlobalShortcut::startListening() {
    qDebug() << "KGlobalAccel ready";
}
