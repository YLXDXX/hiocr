#ifndef SERVICEMANAGER_H
#define SERVICEMANAGER_H

#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QMap>

class ServiceManager : public QObject
{
    Q_OBJECT
public:
    explicit ServiceManager(QObject *parent = nullptr);
    ~ServiceManager();

    // 启动指定 ID 的服务（需外部传入启动命令）
    void startService(const QString& id, const QString& command);

    // 停止指定服务
    void stopService(const QString& id);

    // 停止所有服务
    void stopAllServices();

    // 检查服务是否运行
    bool isServiceRunning(const QString& id) const;

    // 获取所有正在运行的服务数量
    int runningCount() const;

    // 重置空闲计时器
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
    QMap<QString, QProcess*> m_processes; // ID -> Process
    QTimer* m_idleTimer;
};

#endif // SERVICEMANAGER_H
