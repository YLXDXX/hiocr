#include "servicemanager.h"
#include "settingsmanager.h"
#include <QDebug>

#ifdef Q_OS_LINUX
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#endif

ServiceManager::ServiceManager(QObject *parent) : QObject(parent)
{
    // 【修复】初始化空闲计时器
    m_idleTimer = new QTimer(this);
    m_idleTimer->setSingleShot(true); // 单次触发
    connect(m_idleTimer, &QTimer::timeout, this, &ServiceManager::onIdleTimeout);
}

ServiceManager::~ServiceManager()
{
    stopAllServices();
}

void ServiceManager::startService(const QString& id, const QString& command)
{
    if (id.isEmpty() || command.isEmpty()) {
        emit serviceError(id, "ID 或命令为空，无法启动");
        return;
    }

    if (!m_processes.contains(id)) {
        QProcess* process = new QProcess(this);
        process->setProperty("serviceId", id);

        // 连接进程启动信号
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

void ServiceManager::stopService(const QString& id)
{
    if (!m_processes.contains(id)) return;

    QProcess* process = m_processes[id];
    if (process->state() != QProcess::NotRunning) {
        qDebug() << "Stopping service:" << id;

        // 【新增】设置标记，表明这是主动停止，不是意外崩溃
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

void ServiceManager::stopAllServices()
{
    qDebug() << "Stopping all services. Count:" << m_processes.size();
    QStringList ids = m_processes.keys();
    for (const QString& id : ids) {
        stopService(id);
    }
}

bool ServiceManager::isServiceRunning(const QString& id) const
{
    return m_processes.contains(id) && m_processes[id]->state() == QProcess::Running;
}

int ServiceManager::runningCount() const
{
    int count = 0;
    for (auto it = m_processes.begin(); it != m_processes.end(); ++it) {
        if (it.value()->state() == QProcess::Running) count++;
    }
    return count;
}

// 【修复】实现重置计时器逻辑
void ServiceManager::resetIdleTimer()
{
    int timeoutMinutes = SettingsManager::instance()->serviceIdleTimeout();

    // 如果设置为 -1，则禁用空闲关闭功能
    if (timeoutMinutes < 0) {
        m_idleTimer->stop();
        return;
    }

    // 重新启动计时器
    m_idleTimer->start(timeoutMinutes * 60 * 1000);
}

// 【修复】实现空闲超时槽函数
void ServiceManager::onIdleTimeout()
{
    qDebug() << "Service idle timeout reached, stopping ALL services.";
    stopAllServices();
}

// 【修复】修改错误处理，忽略主动杀死导致的错误
void ServiceManager::onProcessError(QProcess::ProcessError error)
{
    QProcess* process = qobject_cast<QProcess*>(sender());
    if (!process) return;

    // 如果是主动终止导致的错误（如管道破裂等），忽略
    if (process->property("intentionallyKilled").toBool()) {
        return;
    }

    QString id = process->property("serviceId").toString();
    emit serviceError(id, process->errorString());
}

void ServiceManager::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    QProcess* process = qobject_cast<QProcess*>(sender());
    if (!process) return;

    QString id = process->property("serviceId").toString();

    // 【修复】只有非主动杀死且状态为崩溃时才报错
    if (status == QProcess::CrashExit) {
        if (!process->property("intentionallyKilled").toBool()) {
            emit serviceError(id, "服务崩溃");
        }
    }

    emit serviceStopped(id);
    emit runningCountChanged(runningCount());
}
