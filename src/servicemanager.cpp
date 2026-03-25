#include "servicemanager.h"
#include "settingsmanager.h"
#include <QDebug>

// 【新增】Linux 特定头文件
#ifdef Q_OS_LINUX
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#endif

ServiceManager::ServiceManager(QObject *parent) : QObject(parent)
{
    m_process = new QProcess(this);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ServiceManager::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &ServiceManager::onProcessError);

    m_idleTimer = new QTimer(this);
    m_idleTimer->setSingleShot(true);
    connect(m_idleTimer, &QTimer::timeout, this, &ServiceManager::onIdleTimeout);
}

ServiceManager::~ServiceManager()
{
    stopService();
}

void ServiceManager::ensureRunning()
{
    if (m_process->state() == QProcess::Running || m_isStarting) {
        emit serviceReady();
        return;
    }

    if (!SettingsManager::instance()->autoStartService()) {
        emit serviceReady();
        return;
    }

    startServiceInternal();
}

void ServiceManager::startServiceInternal()
{
    QString command = SettingsManager::instance()->serviceStartCommand();
    if (command.isEmpty()) {
        emit serviceReady();
        return;
    }

    qDebug() << "Starting OCR service with command:" << command;
    m_isStarting = true;
    m_manualStop = false;

    #ifdef Q_OS_LINUX
    m_process->setChildProcessModifier([]() {

        // 保留进程组设置，这对 stopService() 中的 kill(-pid) 至关重要
        setpgid(0, 0);
    });
    #endif

    m_process->startCommand(command);

    QTimer::singleShot(2000, this, [this](){
        if (m_isStarting) {
            m_isStarting = false;
            if (m_process->state() == QProcess::Running) {
                qDebug() << "Service process started successfully. PID:" << m_process->processId();
                emit runningStateChanged(true);
                emit serviceReady();
            } else {
                qWarning() << "Service process failed to stay running after 2s.";
                emit errorOccurred("服务启动超时或命令执行失败，请检查命令配置。");
                emit runningStateChanged(false);
            }
        }
    });
}

void ServiceManager::stopService()
{
    if (m_process->state() != QProcess::NotRunning) {
        qDebug() << "Stopping OCR service...";

        // 【关键修复】标记为主动停止，屏蔽后续的 Crashed 错误信号
        m_manualStop = true;

        #ifdef Q_OS_LINUX
        pid_t pid = m_process->processId();
        if (pid > 0) {
            // 1. 向整个进程组发送 SIGTERM (优雅退出)
            // 此时 PID == PGID (因为我们在 start 时设置了 setpgid)
            if (kill(-pid, SIGTERM) == 0) {
                // 等待进程退出
                if (!m_process->waitForFinished(3000)) {
                    // 2. 超时后发送 SIGKILL (强制退出)
                    qDebug() << "Service still running, sending SIGKILL to group" << pid;
                    kill(-pid, SIGKILL);
                    m_process->waitForFinished(1000);
                }
            } else {
                // 如果 kill 失败（极少情况），回退到 Qt 默认方式
                qWarning() << "Failed to kill process group" << pid << ", fallback to terminate.";
                m_process->terminate();
                if (!m_process->waitForFinished(3000)) {
                    m_process->kill();
                    m_process->waitForFinished(1000);
                }
            }
        }
        #else
        // Windows/其他平台使用默认方式
        m_process->terminate();
        if (!m_process->waitForFinished(3000)) {
            m_process->kill();
            m_process->waitForFinished(1000);
        }
        #endif
        emit runningStateChanged(false);
    }
    m_idleTimer->stop();
}

bool ServiceManager::isRunning() const
{
    return m_process->state() == QProcess::Running;
}

void ServiceManager::resetIdleTimer()
{
    int timeoutMinutes = SettingsManager::instance()->serviceIdleTimeout();
    if (timeoutMinutes < 0) {
        m_idleTimer->stop();
        return;
    }
    m_idleTimer->start(timeoutMinutes * 60 * 1000);
}

void ServiceManager::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(exitCode);

    // 【修改】将判断提到最前面，如果是主动停止，直接返回，不输出任何日志
    if (m_manualStop) {
        m_manualStop = false; // 重置标志
        return;
    }

    qDebug() << "Service process finished. ExitStatus:" << status;
    m_isStarting = false;

    emit runningStateChanged(false);

    if (status == QProcess::CrashExit) {
        emit errorOccurred("服务进程意外崩溃。");
    }
}

void ServiceManager::onProcessError(QProcess::ProcessError error)
{
    // 【修改】将判断提到最前面，如果是主动停止，直接返回，不输出任何日志
    if (m_manualStop) {
        return;
    }

    qDebug() << "Service process error:" << error << m_process->errorString();
    m_isStarting = false;

    emit errorOccurred(m_process->errorString());
    emit runningStateChanged(false);
}

void ServiceManager::onIdleTimeout()
{
    qDebug() << "Service idle timeout reached, stopping service.";
    stopService();
}
