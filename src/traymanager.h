#ifndef TRAYMANAGER_H
#define TRAYMANAGER_H

#include <QObject>
#include <QSystemTrayIcon>

class QMenu;
class QAction;

class TrayManager : public QObject
{
    Q_OBJECT
public:
    explicit TrayManager(QObject* parent = nullptr);
    void showMessage(const QString& title, const QString& message);

signals:
    void showWindowRequested();
    void screenshotRequested();
    void textRecognizeRequested();
    void formulaRecognizeRequested();
    void tableRecognizeRequested();
    void quitRequested();

private slots:
    void onActivated(QSystemTrayIcon::ActivationReason reason);

private:
    QSystemTrayIcon* m_trayIcon;
    QMenu* m_trayMenu;
};

#endif // TRAYMANAGER_H
