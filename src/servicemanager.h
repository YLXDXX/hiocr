#ifndef SERVICEMANAGER_H
#define SERVICEMANAGER_H

#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QMap>
#include <QDebug>
#include <QVariant>

#ifdef Q_OS_LINUX
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#endif

#include "settingsmanager.h"

class ServiceManager : public QObject
{
    Q_OBJECT
public:
    explicit ServiceManager(QObject *parent = nullptr);
    ~ServiceManager();

    void startService(const QString& id, const QString& command);
    void stopService(const QString& id);
    void stopAllServices();

    bool isServiceRunning(const QString& id) const;
    int runningCount() const;

    void resetIdleTimer();

signals:
    void serviceStarted(const QString& id);
    void serviceStopped(const QString& id);
    void serviceError(const QString& id, const QString& error);
    void runningCountChanged(int count);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessError(QProcess::ProcessError error);
    void onIdleTimeout();

private:
    QMap<QString, QProcess*> m_processes;
    QTimer* m_idleTimer;
};

// --- Implementation ---

inline ServiceManager::ServiceManager(QObject *parent) : QObject(parent)
{
    m_idleTimer = new QTimer(this);
    m_idleTimer->setSingleShot(true);
    connect(m_idleTimer, &QTimer::timeout, this, &ServiceManager::onIdleTimeout);
}

inline ServiceManager::~ServiceManager()
{
    stopAllServices();
}

inline void ServiceManager::startService(const QString& id, const QString& command)
{
    if (id.isEmpty() || command.isEmpty()) {
        emit serviceError(id, "ID 或命令为空，无法启动");
        return;
    }

    if (!m_processes.contains(id)) {
        QProcess* process = new QProcess(this);
        process->setProperty("serviceId", id);

        connect(process, &QProcess::started, this, [this, id](){
            qDebug() << "Service process started successfully:" << id;
            emit serviceStarted(id);
            emit runningCountChanged(runningCount());
        });

        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &ServiceManager::onProcessFinished);
        connect(process, &QProcess::errorOccurred, this, &ServiceManager::onProcessError);
        m_processes[id] = process;
    }

    QProcess* process = m_processes[id];

    if (process->state() == QProcess::Running) {
        emit serviceStarted(id);
        return;
    }

    qDebug() << "Starting service:" << id << command;

    #ifdef Q_OS_LINUX
    process->setChildProcessModifier([](){
        setpgid(0, 0);
    });
    #endif

    process->startCommand(command);
}

inline void ServiceManager::stopService(const QString& id)
{
    if (!m_processes.contains(id)) return;

    QProcess* process = m_processes[id];
    if (process->state() != QProcess::NotRunning) {
        qDebug() << "Stopping service:" << id;

        process->setProperty("intentionallyKilled", true);

        #ifdef Q_OS_LINUX
        pid_t pid = process->processId();
        if (pid > 0) kill(-pid, SIGTERM);
        if (!process->waitForFinished(3000)) {
            if (pid > 0) kill(-pid, SIGKILL);
            process->waitForFinished(1000);
        }
        #else
        process->terminate();
        if (!process->waitForFinished(3000)) process->kill();
        #endif
    }
}

inline void ServiceManager::stopAllServices()
{
    qDebug() << "Stopping all services. Count:" << m_processes.size();
    QStringList ids = m_processes.keys();
    for (const QString& id : ids) {
        stopService(id);
    }
}

inline bool ServiceManager::isServiceRunning(const QString& id) const
{
    return m_processes.contains(id) && m_processes[id]->state() == QProcess::Running;
}

inline int ServiceManager::runningCount() const
{
    int count = 0;
    for (auto it = m_processes.begin(); it != m_processes.end(); ++it) {
        if (it.value()->state() == QProcess::Running) count++;
    }
    return count;
}

inline void ServiceManager::resetIdleTimer()
{
    int timeoutMinutes = SettingsManager::instance()->serviceIdleTimeout();

    if (timeoutMinutes < 0) {
        m_idleTimer->stop();
        return;
    }

    m_idleTimer->start(timeoutMinutes * 60 * 1000);
}

inline void ServiceManager::onIdleTimeout()
{
    qDebug() << "Service idle timeout reached, stopping ALL services.";
    stopAllServices();
}

inline void ServiceManager::onProcessError(QProcess::ProcessError error)
{
    QProcess* process = qobject_cast<QProcess*>(sender());
    if (!process) return;

    if (process->property("intentionallyKilled").toBool()) {
        return;
    }

    QString id = process->property("serviceId").toString();
    emit serviceError(id, process->errorString());
}

inline void ServiceManager::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    QProcess* process = qobject_cast<QProcess*>(sender());
    if (!process) return;

    QString id = process->property("serviceId").toString();

    if (status == QProcess::CrashExit) {
        if (!process->property("intentionallyKilled").toBool()) {
            emit serviceError(id, "服务崩溃");
        }
    }

    emit serviceStopped(id);
    emit runningCountChanged(runningCount());
}

#endif // SERVICEMANAGER_H
