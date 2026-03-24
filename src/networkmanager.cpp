#include "networkmanager.h"
#include "settingsmanager.h" // 引入 SettingsManager

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QHttpMultiPart>
#include <QDebug>

NetworkManager::NetworkManager(QObject* parent) : QObject(parent) {
    m_manager = new QNetworkAccessManager(this);

    // 【优化完成】不再使用 QSettings，而是从 SettingsManager 获取
    m_serverUrl = SettingsManager::instance()->serverUrl();

    // 【优化完成】连接设置变化信号，动态更新 URL
    connect(SettingsManager::instance(), &SettingsManager::serverUrlChanged, this, [this](const QString& url){
        m_serverUrl = url;
    });
}


void NetworkManager::setServerUrl(const QString& url) {
    if (m_serverUrl == url) return;
    m_serverUrl = url;
    // 注意：这里不需要再设置 QSettings，因为设置来源已经是 SettingsManager 了
    // 这个 setServerUrl 现在可能变得多余，通常应该由 SettingsManager 驱动
    // 但如果需要保留作为本地缓存，也可以保留
}

void NetworkManager::sendRequest(const QString& base64Image,
                                 const QString& prompt,
                                 const QString& serverUrl)
{
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
    }

    // 确定实际使用的 URL：优先使用传入的参数，否则使用成员变量 m_serverUrl
    QString url = serverUrl.isEmpty() ? m_serverUrl : serverUrl;

    // ... 后续代码保持不变 ...
    QJsonArray messages;
    QJsonObject userMessage;
    QJsonArray content;

    QJsonObject textPart;
    textPart["type"] = "text";
    textPart["text"] = prompt;
    content.append(textPart);

    QJsonObject imagePart;
    imagePart["type"] = "image_url";
    QJsonObject imageUrlObj;
    imageUrlObj["url"] = "data:image/png;base64," + base64Image;
    imagePart["image_url"] = imageUrlObj;
    content.append(imagePart);

    userMessage["role"] = "user";
    userMessage["content"] = content;
    messages.append(userMessage);

    QJsonObject json;
    json["messages"] = messages;
    json["stream"] = false;
    json["cache_prompt"] = false;
    json["temperature"] = 0.3;
    json["max_tokens"] = 2048;
    json["seed"] = -1;

    QJsonDocument doc(json);
    QByteArray data = doc.toJson();

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    m_currentReply = m_manager->post(request, data);
    connect(m_currentReply, &QNetworkReply::finished, this, &NetworkManager::onReplyFinished);
}

void NetworkManager::onReplyFinished()
{
    if (!m_currentReply) return;

    if (m_currentReply->error() != QNetworkReply::NoError) {
        emit requestFinished(QString(), false, m_currentReply->errorString());
    } else {
        QByteArray response = m_currentReply->readAll();
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            emit requestFinished(QString(), false, "JSON解析错误: " + parseError.errorString());
        } else {
            QJsonObject obj = doc.object();

            // 优先检查服务端是否返回 error 字段（如 OpenAI 兼容服务）
            if (obj.contains("error")) {
                QJsonValue errorVal = obj["error"];
                QString errorMsg;
                if (errorVal.isObject()) {
                    errorMsg = errorVal.toObject()["message"].toString();
                } else if (errorVal.isString()) {
                    errorMsg = errorVal.toString();
                }
                if (errorMsg.isEmpty())
                    errorMsg = "Unknown server error";
                emit requestFinished(QString(), false, errorMsg);
                return;
            }

            // 处理正常的 OpenAI 兼容响应
            if (obj.contains("choices") && obj["choices"].isArray()) {
                QJsonArray choices = obj["choices"].toArray();
                if (!choices.isEmpty()) {
                    QJsonObject choice = choices[0].toObject();
                    if (choice.contains("message")) {
                        QJsonObject message = choice["message"].toObject();
                        QString content = message["content"].toString();
                        emit requestFinished(content, true, QString());
                        return;
                    } else if (choice.contains("text")) {
                        QString content = choice["text"].toString();
                        emit requestFinished(content, true, QString());
                        return;
                    }
                }
            }
            emit requestFinished(QString(), false, "响应中缺少预期的字段");
        }
    }

    m_currentReply->deleteLater();
    m_currentReply = nullptr;
}
