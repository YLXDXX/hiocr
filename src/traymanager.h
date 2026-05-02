#ifndef TRAYMANAGER_H
#define TRAYMANAGER_H

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QApplication>
#include <QStyle>

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
    void notificationClicked();

private slots:
    void onActivated(QSystemTrayIcon::ActivationReason reason);

private:
    QSystemTrayIcon* m_trayIcon;
    QMenu* m_trayMenu;
};

// --- Implementation ---

inline TrayManager::TrayManager(QObject* parent) : QObject(parent)
{
    m_trayIcon = new QSystemTrayIcon(this);
    QIcon icon = QIcon::fromTheme("hiocr", QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));
    m_trayIcon->setIcon(icon);
    m_trayIcon->setToolTip("hiocr");

    m_trayMenu = new QMenu();

    QAction* showAction = m_trayMenu->addAction("显示窗口");
    connect(showAction, &QAction::triggered, this, &TrayManager::showWindowRequested);
    m_trayMenu->addSeparator();

    QAction* textAction = m_trayMenu->addAction("文字识别");
    connect(textAction, &QAction::triggered, this, &TrayManager::textRecognizeRequested);
    QAction* formulaAction = m_trayMenu->addAction("公式识别");
    connect(formulaAction, &QAction::triggered, this, &TrayManager::formulaRecognizeRequested);
    QAction* tableAction = m_trayMenu->addAction("表格识别");
    connect(tableAction, &QAction::triggered, this, &TrayManager::tableRecognizeRequested);
    m_trayMenu->addSeparator();

    QAction* quitAction = m_trayMenu->addAction("退出");
    connect(quitAction, &QAction::triggered, this, &TrayManager::quitRequested);

    m_trayIcon->setContextMenu(m_trayMenu);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &TrayManager::onActivated);
    connect(m_trayIcon, &QSystemTrayIcon::messageClicked, this, &TrayManager::notificationClicked);

    m_trayIcon->show();
}

inline void TrayManager::showMessage(const QString& title, const QString& message) {
    m_trayIcon->showMessage(title, message);
}

inline void TrayManager::onActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
        emit screenshotRequested();
}

#endif // TRAYMANAGER_H
