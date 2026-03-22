#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class NetworkManager : public QObject {
    Q_OBJECT
public:
    explicit NetworkManager(QObject* parent = nullptr);

    // 发送多模态请求，serverUrl 可选，若不传则使用成员变量 m_serverUrl
    void sendRequest(const QString& base64Image,
                     const QString& prompt,
                     const QString& serverUrl = QString());

    // 设置服务器 URL（同时保存到 QSettings）
    void setServerUrl(const QString& url);

    // 获取当前服务器 URL
    QString serverUrl() const { return m_serverUrl; }

signals:
    void requestFinished(const QString& result, bool success, const QString& error);

private slots:
    void onReplyFinished();

private:
    QNetworkAccessManager* m_manager;
    QNetworkReply* m_currentReply = nullptr;
    QString m_serverUrl;   // 存储可配置的服务器 URL
};

#endif // NETWORKMANAGER_H
