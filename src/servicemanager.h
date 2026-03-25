#ifndef SERVICEMANAGER_H
#define SERVICEMANAGER_H

#include <QObject>
#include <QProcess>
#include <QTimer>

class ServiceManager : public QObject
{
    Q_OBJECT
public:
    explicit ServiceManager(QObject *parent = nullptr);

    // 尝试启动服务（如果未运行且配置了自动启动）
    void ensureRunning();

    // 重置空闲计时器（在有活动时调用）
    void resetIdleTimer();

    // 手动停止服务
    void stopService();

signals:
    // 服务启动完成（无论成功还是跳过，都发出此信号以通知 Controller 继续）
    void serviceReady();
    // 服务启动失败或运行出错
    void errorOccurred(const QString& error);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessError(QProcess::ProcessError error);
    void onIdleTimeout();

private:
    void startServiceInternal();

    QProcess* m_process = nullptr;
    QTimer* m_idleTimer = nullptr;
    bool m_isStarting = false;
};

#endif // SERVICEMANAGER_H
