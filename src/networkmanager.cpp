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

    // 【新增】初始化定时器
    m_timeoutTimer = new QTimer(this);
    m_timeoutTimer->setSingleShot(true); // 单次触发
    connect(m_timeoutTimer, &QTimer::timeout, this, [this](){
        if (m_currentReply) {
            qWarning() << "Network request timed out, aborting...";
            m_currentReply->abort(); // 中断请求
        }
    });
}

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

    // 【新增】设置模型名称 (如果配置中有)
    if (!config.model.isEmpty()) {
        json["model"] = config.model;
    }

    // 合并用户自定义参数 (高级参数)
    QString paramsJsonStr = SettingsManager::instance()->requestParameters();
    if (!paramsJsonStr.isEmpty()) {
        QJsonParseError err;
        QJsonDocument paramsDoc = QJsonDocument::fromJson(paramsJsonStr.toUtf8(), &err);
        if (err.error == QJsonParseError::NoError && paramsDoc.isObject()) {
            QJsonObject paramsObj = paramsDoc.object();
            for (auto it = paramsObj.constBegin(); it != paramsObj.constEnd(); ++it) {
                // 不覆盖已设置的值，除非用户明确要覆盖
                // 注意：enable_thinking 通常需要在最外层，这里逻辑正确
                if (!json.contains(it.key())) {
                    json[it.key()] = it.value();
                }
            }
        }
    }

    // --- 3. 构建网络请求 ---
    QJsonDocument doc(json);
    QByteArray data = doc.toJson();

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // 【新增】支持 API Key 鉴权
    if (!config.apiKey.isEmpty()) {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(config.apiKey).toUtf8());
    }

    m_currentReply = m_manager->post(request, data);

    // 【新增】启动超时计时器
    // 从 SettingsManager 读取超时秒数并转换为毫秒
    int timeoutSec = SettingsManager::instance()->requestTimeout();
    m_timeoutTimer->start(timeoutSec * 1000);

    connect(m_currentReply, &QNetworkReply::finished, this, &NetworkManager::onReplyFinished);
}

void NetworkManager::onReplyFinished()
{
    // 【新增】请求结束（无论成功失败），停止计时器
    m_timeoutTimer->stop();

    if (!m_currentReply) return;

    QNetworkReply* reply = m_currentReply;
    m_currentReply = nullptr; // 清空成员指针，后面通过 reply 操作

    // 【新增】检测是否为超时导致的中断
    if (reply->error() == QNetworkReply::OperationCanceledError) {
        emit requestFinished(QString(), false, "请求超时，已强制中断");
        reply->deleteLater();
        return;
    }


    // 检查网络错误
    if (reply->error() != QNetworkReply::NoError) {
        QString errorString = reply->errorString();

        // 尝试读取返回的错误详情 (OpenAI 格式)
        QByteArray errorData = reply->readAll();
        QJsonParseError parseErr;
        QJsonDocument errDoc = QJsonDocument::fromJson(errorData, &parseErr);
        if (parseErr.error == QJsonParseError::NoError && errDoc.isObject()) {
            QJsonObject errObj = errDoc.object();
            if (errObj.contains("error")) {
                QJsonObject errDetail = errObj["error"].toObject();
                errorString = errDetail["message"].toString(errorString);
            }
        }

        emit requestFinished(QString(), false, "网络错误: " + errorString);
        reply->deleteLater();
        return;
    }

    // 【关键修复】解析成功的返回数据
    QByteArray response = reply->readAll();
    reply->deleteLater();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        emit requestFinished(QString(), false, "JSON解析错误: " + parseError.errorString());
        return;
    }

    QJsonObject obj = doc.object();

    // 解析 OpenAI 格式的返回结果
    // 结构: { "choices": [ { "message": { "content": "..." } } ] }
    if (obj.contains("choices") && obj["choices"].isArray()) {
        QJsonArray choices = obj["choices"].toArray();
        if (!choices.isEmpty()) {
            QJsonObject firstChoice = choices[0].toObject();
            if (firstChoice.contains("message")) {
                QJsonObject message = firstChoice["message"].toObject();
                QString content = message["content"].toString();

                // 成功获取内容
                emit requestFinished(content, true, QString());
                return;
            }
        }
    }

    // 如果结构不匹配或为空，报错
    emit requestFinished(QString(), false, "响应格式无效或无有效内容: " + QString::fromUtf8(response.left(200)));
}
