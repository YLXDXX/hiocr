#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QDebug>
#include "settingsmanager.h"

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
    void abortRequest();

signals:
    void requestFinished(const QString& result, bool success, const QString& error);
    void streamDataReceived(const QString& delta);

private slots:
    void onReplyFinished();
    void onReadyRead();

private:
    QNetworkAccessManager* m_manager;
    QNetworkReply* m_currentReply = nullptr;
    QTimer* m_timeoutTimer = nullptr;

    bool m_isCurrentStream = false;
    QByteArray m_rawResponseData;
    QByteArray m_streamBuffer;
};

// --- Implementation ---

inline NetworkManager::NetworkManager(QObject* parent) : QObject(parent) {
    m_manager = new QNetworkAccessManager(this);

    m_timeoutTimer = new QTimer(this);
    m_timeoutTimer->setSingleShot(true);
    connect(m_timeoutTimer, &QTimer::timeout, this, [this](){
        if (m_currentReply) {
            qWarning() << "Network request timed out, aborting...";
            m_currentReply->abort();
        }
    });
}

inline void NetworkManager::sendRequest(const RequestConfig& config)
{
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }

    m_rawResponseData.clear();
    m_streamBuffer.clear();
    m_isCurrentStream = false;

    QString url = config.serverUrl.isEmpty() ? SettingsManager::instance()->serverUrl() : config.serverUrl;

    QJsonArray messages;
    QJsonObject userMessage;
    QJsonArray content;

    QJsonObject textPart;
    textPart["type"] = "text";
    textPart["text"] = config.prompt;
    content.append(textPart);

    QJsonObject imagePart;
    imagePart["type"] = "image_url";
    QJsonObject imageUrlObj;
    imageUrlObj["url"] = "data:image/png;base64," + config.base64Image;
    imagePart["image_url"] = imageUrlObj;
    content.append(imagePart);

    userMessage["role"] = "user";
    userMessage["content"] = content;
    messages.append(userMessage);

    QJsonObject json;
    json["messages"] = messages;

    if (!config.model.isEmpty()) {
        json["model"] = config.model;
    }

    json["stream"] = false;

    QString paramsJsonStr = SettingsManager::instance()->requestParameters();
    if (!paramsJsonStr.isEmpty()) {
        QJsonParseError err;
        QJsonDocument paramsDoc = QJsonDocument::fromJson(paramsJsonStr.toUtf8(), &err);
        if (err.error == QJsonParseError::NoError && paramsDoc.isObject()) {
            QJsonObject paramsObj = paramsDoc.object();
            for (auto it = paramsObj.constBegin(); it != paramsObj.constEnd(); ++it) {
                json[it.key()] = it.value();
            }
        }
    }

    m_isCurrentStream = json["stream"].toBool();
    qDebug() << "Request mode: Stream =" << m_isCurrentStream;

    QJsonDocument doc(json);
    QByteArray data = doc.toJson();

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!config.apiKey.isEmpty()) {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(config.apiKey).toUtf8());
    }

    m_currentReply = m_manager->post(request, data);

    if (m_isCurrentStream) {
        connect(m_currentReply, &QNetworkReply::readyRead, this, &NetworkManager::onReadyRead);
    }

    connect(m_currentReply, &QNetworkReply::finished, this, &NetworkManager::onReplyFinished);

    int timeoutSec = SettingsManager::instance()->requestTimeout();
    m_timeoutTimer->start(timeoutSec * 1000);
}

inline void NetworkManager::onReadyRead()
{
    if (!m_currentReply || !m_isCurrentStream) return;

    m_timeoutTimer->start(SettingsManager::instance()->requestTimeout() * 1000);

    m_streamBuffer.append(m_currentReply->readAll());

    while (true) {
        int newlinePos = m_streamBuffer.indexOf('\n');
        if (newlinePos == -1) {
            break;
        }

        QByteArray line = m_streamBuffer.left(newlinePos).trimmed();
        m_streamBuffer.remove(0, newlinePos + 1);

        if (line.isEmpty()) continue;

        if (line.startsWith("data: ")) {
            QByteArray jsonData = line.mid(6);
            if (jsonData == "[DONE]") return;

            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(jsonData, &err);

            if (err.error == QJsonParseError::NoError && doc.isObject()) {
                QJsonObject obj = doc.object();
                if (obj.contains("choices")) {
                    QJsonArray choices = obj["choices"].toArray();
                    if (!choices.isEmpty()) {
                        QJsonObject delta = choices[0].toObject()["delta"].toObject();
                        if (delta.contains("content")) {
                            QString content = delta["content"].toString();
                            if (!content.isEmpty()) {
                                emit streamDataReceived(content);
                            }
                        }
                    }
                }
            }
        }
    }
}

inline void NetworkManager::onReplyFinished()
{
    m_timeoutTimer->stop();
    if (!m_currentReply) return;

    QNetworkReply* reply = m_currentReply;
    m_currentReply = nullptr;

    if (reply->error() != QNetworkReply::NoError) {
        QString errorStr = reply->errorString();
        if (reply->error() == QNetworkReply::OperationCanceledError) {
            errorStr = "请求超时或已取消";
        } else {
            QByteArray errorData = reply->readAll();
            QJsonDocument errDoc = QJsonDocument::fromJson(errorData);
            if (errDoc.isObject() && errDoc.object().contains("error")) {
                errorStr = errDoc.object()["error"].toObject()["message"].toString();
            }
        }
        emit requestFinished(QString(), false, errorStr);
        reply->deleteLater();
        return;
    }

    if (m_isCurrentStream) {
        emit requestFinished(QString(), true, QString());
    }
    else {
        QByteArray response = reply->readAll();
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            emit requestFinished(QString(), false, "JSON解析错误: " + parseError.errorString());
        } else {
            QJsonObject obj = doc.object();
            QString content;
            if (obj.contains("choices")) {
                QJsonArray choices = obj["choices"].toArray();
                if (!choices.isEmpty()) {
                    QJsonObject message = choices[0].toObject()["message"].toObject();
                    content = message["content"].toString();
                }
            }

            if (content.isEmpty()) {
                emit requestFinished(QString(), false, "响应无效或无内容");
            } else {
                emit requestFinished(content, true, QString());
            }
        }
    }

    reply->deleteLater();
}


inline void NetworkManager::abortRequest()
{
    m_timeoutTimer->stop();
    if (m_currentReply) {
        m_currentReply->abort();
    }
    m_streamBuffer.clear();
}

#endif // NETWORKMANAGER_H
