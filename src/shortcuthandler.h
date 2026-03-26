#ifndef SHORTCUTHANDLER_H
#define SHORTCUTHANDLER_H

#include <QObject>

class QShortcut;

class ShortcutHandler : public QObject
{
    Q_OBJECT
public:
    explicit ShortcutHandler(QObject *parent = nullptr);

    void applyShortcuts();

signals:
    void screenshotRequested();
    void textRecognizeRequested();
    void formulaRecognizeRequested();
    void tableRecognizeRequested();
    void externalProcessRequested();

private:
    void setupLocalShortcuts();
    void setupGlobalShortcuts();

    QShortcut* m_scScreenshot = nullptr;
    QShortcut* m_scText = nullptr;
    QShortcut* m_scFormula = nullptr;
    QShortcut* m_scTable = nullptr;
    QShortcut* m_scExternalProcess = nullptr;
};

#endif // SHORTCUTHANDLER_H
