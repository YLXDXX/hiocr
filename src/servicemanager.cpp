#include "servicemanager.h"
#include "settingsmanager.h"
#include <QDebug>

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

void ServiceManager::ensureRunning()
{
    // 如果进程已经在运行或正在启动中，直接认为已就绪
    if (m_process->state() == QProcess::Running || m_isStarting) {
        emit serviceReady();
        return;
    }

    // 检查设置是否启用自动启动
    if (!SettingsManager::instance()->autoStartService()) {
        // 用户禁用了自动启动，直接返回 Ready 让业务逻辑继续（业务逻辑会处理连接失败的错误）
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

    // 使用 startCommand (Qt 6) 或 split + start (Qt 5)
    // 这里假设是 Qt 6 环境
    m_process->startCommand(command);

    // 给服务一点时间启动，这里简单处理：启动后等待一小会儿发出 ready 信号
    // 更高级的做法是轮询健康检查端点，这里为了简单使用延时策略
    QTimer::singleShot(2000, this, [this](){
        if (m_process->state() == QProcess::Running) {
            qDebug() << "Service process started successfully.";
            m_isStarting = false;
            emit serviceReady();
        } else {
            // 如果进程启动失败或立即退出了，onProcessError 会处理
            // 这里防止卡死
            m_isStarting = false;
        }
    });
}

void ServiceManager::stopService()
{
    if (m_process->state() != QProcess::NotRunning) {
        qDebug() << "Stopping OCR service...";
        m_process->terminate();
        // 如果 3 秒后还没关掉，强制 Kill
        if (!m_process->waitForFinished(3000)) {
            m_process->kill();
        }
    }
    m_idleTimer->stop();
}

void ServiceManager::resetIdleTimer()
{
    int timeoutMinutes = SettingsManager::instance()->serviceIdleTimeout();

    // -1 代表永不自动关闭
    if (timeoutMinutes < 0) {
        m_idleTimer->stop();
        return;
    }

    m_idleTimer->start(timeoutMinutes * 60 * 1000); // 转换为毫秒
}

void ServiceManager::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(exitCode);
    Q_UNUSED(status);
    qDebug() << "Service process finished.";
    m_isStarting = false;
}

void ServiceManager::onProcessError(QProcess::ProcessError error)
{
    qDebug() << "Service process error:" << error << m_process->errorString();
    m_isStarting = false;
    emit errorOccurred(m_process->errorString());
}

void ServiceManager::onIdleTimeout()
{
    qDebug() << "Service idle timeout reached, stopping service.";
    stopService();
}
