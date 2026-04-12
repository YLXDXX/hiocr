#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>

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
    void sendRequest(const RequestConfig& config);
    void abortRequest();  // 【新增】强制中止当前请求

signals:
    // 标准结束信号
    void requestFinished(const QString& result, bool success, const QString& error);

    // 【新增】流式数据增量到达信号
    void streamDataReceived(const QString& delta);

private slots:
    void onReplyFinished();
    // 【新增】处理流式数据到达的槽函数
    void onReadyRead();

private:
    QNetworkAccessManager* m_manager;
    QNetworkReply* m_currentReply = nullptr;
    QTimer* m_timeoutTimer = nullptr;

    // 【新增】标记当前请求是否为流式模式
    bool m_isCurrentStream = false;
    // 【新增】缓冲非流式请求的完整响应数据
    QByteArray m_rawResponseData;

    QByteArray m_streamBuffer;
};

#endif // NETWORKMANAGER_H
