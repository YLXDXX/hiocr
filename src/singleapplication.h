// src/singleapplication.h
#ifndef SINGLEAPPLICATION_H
#define SINGLEAPPLICATION_H

#include <QApplication> // 【修改】引入 QApplication
#include <QLocalServer>

class SingleApplication : public QApplication // 【修改】继承 QApplication
{
    Q_OBJECT
public:
    // 【修改】构造函数签名与 QApplication 一致，增加 key 参数
    SingleApplication(int &argc, char **argv, const QString& key);
    ~SingleApplication();

    bool isRunning() const;
    bool sendMessage(const QByteArray &message);

signals:
    void messageReceived(const QByteArray &message);

private slots:
    void onNewConnection();

private:
    QString m_key;
    QLocalServer *m_server = nullptr;
    bool m_isRunning = false;
};

#endif // SINGLEAPPLICATION_H
