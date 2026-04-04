// src/networkmanager.h
#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>

// 【新增】请求配置结构体，方便后续扩展 API Key 等
struct RequestConfig {
    QString serverUrl;
    QString prompt;
    QString base64Image;
    QString apiKey;
    QString model;
};

class NetworkManager : public QObject {
    Q_OBJECT
public:
    explicit NetworkManager(QObject* parent = nullptr);

    // 修改接口，使用结构体传参，虽然目前只用到了部分字段
    void sendRequest(const RequestConfig& config);

    // 保留旧接口兼容性，或者重构所有调用方。这里选择重构调用方。
    // void sendRequest(const QString& base64Image, const QString& prompt, const QString& serverUrl = QString()); // 删除或标记废弃

signals:
    void requestFinished(const QString& result, bool success, const QString& error);

private slots:
    void onReplyFinished();

private:
    QNetworkAccessManager* m_manager;
    QNetworkReply* m_currentReply = nullptr;
    QTimer* m_timeoutTimer = nullptr; // 【新增】超时定时器
};

#endif // NETWORKMANAGER_H
