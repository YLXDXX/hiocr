#ifndef KDEGLOBALSHORTCUT_H
#define KDEGLOBALSHORTCUT_H

#include <QObject>
#include <QMap>

class QAction;

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

#endif // KDEGLOBALSHORTCUT_H
