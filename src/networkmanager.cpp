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

    // --- 1. 构建消息结构 ---
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

    // --- 2. 构建 JSON 请求体 ---
    QJsonObject json;

    // 设置基础参数
    json["messages"] = messages;
    json["stream"] = false; // 默认不使用流式，稍后会被配置覆盖（但会被强制逻辑改回）

    // --- 3. 从配置中读取并合并参数 ---
    // 获取用户配置的 JSON 字符串
    QString paramsJsonStr = SettingsManager::instance()->requestParameters();
    if (!paramsJsonStr.isEmpty()) {
        QJsonParseError err;
        QJsonDocument paramsDoc = QJsonDocument::fromJson(paramsJsonStr.toUtf8(), &err);

        if (err.error == QJsonParseError::NoError && paramsDoc.isObject()) {
            QJsonObject paramsObj = paramsDoc.object();
            // 遍历配置中的参数并合并到请求体 json 中
            // 这会覆盖上面设置的默认值 (如 stream=false，如果用户配置了 stream=true)
            for (auto it = paramsObj.constBegin(); it != paramsObj.constEnd(); ++it) {
                json[it.key()] = it.value();
            }
        } else {
            qWarning() << "Failed to parse request parameters JSON, ignoring. Error:" << err.errorString();
        }
    }

    // --- 4. 强制安全限制 ---
    // 无论用户配置如何，当前 NetworkManager 的 onReplyFinished 逻辑不支持流式解析，
    // 必须强制 stream = false，否则会导致解析错误。
    if (json.contains("stream") && json["stream"].toBool() == true) {
        qWarning() << "Stream=true in config is not supported by current NetworkManager logic, forcing to false.";
        json["stream"] = false;
    }

    // --- 5. 发送请求 ---
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
