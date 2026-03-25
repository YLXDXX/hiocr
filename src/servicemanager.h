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
    ~ServiceManager();

    void ensureRunning();
    void resetIdleTimer();
    void stopService();
    bool isRunning() const;

signals:
    void serviceReady();
    void errorOccurred(const QString& error);
    void runningStateChanged(bool running);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessError(QProcess::ProcessError error);
    void onIdleTimeout();

private:
    void startServiceInternal();

    QProcess* m_process = nullptr;
    QTimer* m_idleTimer = nullptr;
    bool m_isStarting = false;
    bool m_manualStop = false; // 【新增】标记是否为用户主动停止
};

#endif // SERVICEMANAGER_H
