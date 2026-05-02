// src/singleapplication.h
#ifndef SINGLEAPPLICATION_H
#define SINGLEAPPLICATION_H

#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QDebug>

class SingleApplication : public QApplication
{
    Q_OBJECT
public:
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

// --- Implementation ---

inline SingleApplication::SingleApplication(int &argc, char **argv, const QString& key)
: QApplication(argc, argv), m_key(key)
{
    QLocalSocket socket;
    socket.connectToServer(m_key, QLocalSocket::WriteOnly);
    if (socket.waitForConnected(500)) {
        m_isRunning = true;
        return;
    }

    m_server = new QLocalServer(this);
    connect(m_server, &QLocalServer::newConnection, this, &SingleApplication::onNewConnection);

    QLocalServer::removeServer(m_key);
    if (!m_server->listen(m_key)) {
        qCritical() << "Failed to start single instance server:" << m_server->errorString();
    }
    m_isRunning = false;
}

inline SingleApplication::~SingleApplication()
{
    if (m_server) {
        m_server->close();
    }
}

inline bool SingleApplication::isRunning() const
{
    return m_isRunning;
}

inline bool SingleApplication::sendMessage(const QByteArray &message)
{
    if (!m_isRunning) return false;

    QLocalSocket socket;
    socket.connectToServer(m_key, QLocalSocket::WriteOnly);
    if (!socket.waitForConnected(1000)) {
        qWarning() << "Could not connect to running instance";
        return false;
    }

    socket.write(message);
    socket.waitForBytesWritten(1000);
    socket.disconnectFromServer();
    return true;
}

inline void SingleApplication::onNewConnection()
{
    QLocalSocket *socket = m_server->nextPendingConnection();
    if (!socket) return;

    if (socket->waitForReadyRead(1000)) {
        QByteArray data = socket->readAll();
        emit messageReceived(data);
    }
    socket->deleteLater();
}

#endif // SINGLEAPPLICATION_H
