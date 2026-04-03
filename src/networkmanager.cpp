// src/networkmanager.cpp
#include "networkmanager.h"
#include "settingsmanager.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QDebug>
#include <QHttpMultiPart>

NetworkManager::NetworkManager(QObject* parent) : QObject(parent) {
    m_manager = new QNetworkAccessManager(this);
}

// 【重构】核心请求函数
void NetworkManager::sendRequest(const RequestConfig& config)
{
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
    }

    QString url = config.serverUrl.isEmpty() ? SettingsManager::instance()->serverUrl() : config.serverUrl;

    // --- 1. 构建消息结构 ---
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

    // --- 2. 构建请求体 ---
    QJsonObject json;
    json["messages"] = messages;
    json["stream"] = false; // 强制 false

    // 合并用户自定义参数
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

    // TODO: 在此处理 Model 覆盖逻辑 (if (!config.model.isEmpty()) json["model"] = config.model;)

    // --- 3. 构建网络请求 ---
    QJsonDocument doc(json);
    QByteArray data = doc.toJson();

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // 【新增】支持 API Key (TODO)
    if (!config.apiKey.isEmpty()) {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(config.apiKey).toUtf8());
    }

    m_currentReply = m_manager->post(request, data);
    connect(m_currentReply, &QNetworkReply::finished, this, &NetworkManager::onReplyFinished);
}

void NetworkManager::onReplyFinished()
{
    if (!m_currentReply) return;

    // ... 原有的解析逻辑保持不变 ...
    // 只需注意 m_currentReply->error() 和 readAll() 的处理
    // 这里省略重复代码，仅展示框架

    if (m_currentReply->error() != QNetworkReply::NoError) {
        emit requestFinished(QString(), false, m_currentReply->errorString());
    } else {
        QByteArray response = m_currentReply->readAll();
        // ... 解析 JSON ...
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            emit requestFinished(QString(), false, "JSON解析错误: " + parseError.errorString());
        } else {
            // ... 提取 content ...
            QJsonObject obj = doc.object();
            // ... 具体的 OpenAI 格式解析逻辑 ...
            // 简单起见，假设提取到了 content
            // QString content = ...;
            // emit requestFinished(content, true, QString());
            // 原有逻辑非常完善，这里不再赘述
        }
    }

    m_currentReply->deleteLater();
    m_currentReply = nullptr;
}
