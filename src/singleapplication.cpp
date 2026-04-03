// src/singleapplication.cpp
#include "singleapplication.h"
#include <QLocalSocket>
#include <QDebug>

SingleApplication::SingleApplication(int &argc, char **argv, const QString& key)
: QApplication(argc, argv), m_key(key) // 【修改】调用 QApplication 构造函数
{
    // 尝试连接到现有的服务端
    QLocalSocket socket;
    socket.connectToServer(m_key, QLocalSocket::WriteOnly);
    if (socket.waitForConnected(500)) {
        m_isRunning = true;
        return;
    }

    // 连接失败，说明是第一个实例，创建服务端
    m_server = new QLocalServer(this);
    connect(m_server, &QLocalServer::newConnection, this, &SingleApplication::onNewConnection);

    // 移除可能残留的 socket 文件
    QLocalServer::removeServer(m_key);
    if (!m_server->listen(m_key)) {
        qCritical() << "Failed to start single instance server:" << m_server->errorString();
    }
    m_isRunning = false;
}

SingleApplication::~SingleApplication()
{
    if (m_server) {
        m_server->close();
    }
}

bool SingleApplication::isRunning() const
{
    return m_isRunning;
}

bool SingleApplication::sendMessage(const QByteArray &message)
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

void SingleApplication::onNewConnection()
{
    QLocalSocket *socket = m_server->nextPendingConnection();
    if (!socket) return;

    if (socket->waitForReadyRead(1000)) {
        QByteArray data = socket->readAll();
        emit messageReceived(data);
    }
    socket->deleteLater();
}
