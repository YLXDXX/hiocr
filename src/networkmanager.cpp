#include "networkmanager.h"
#include "settingsmanager.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QDebug>

NetworkManager::NetworkManager(QObject* parent) : QObject(parent) {
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

void NetworkManager::sendRequest(const RequestConfig& config)
{
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }

    // 清空缓冲
    m_rawResponseData.clear();
    m_isCurrentStream = false;

    QString url = config.serverUrl.isEmpty() ? SettingsManager::instance()->serverUrl() : config.serverUrl;

    // --- 1. 构建请求体 ---
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

    // --- 2. 合并用户自定义参数并判断是否为流式 ---
    // 默认 stream 为 false
    json["stream"] = false;

    QString paramsJsonStr = SettingsManager::instance()->requestParameters();
    if (!paramsJsonStr.isEmpty()) {
        QJsonParseError err;
        QJsonDocument paramsDoc = QJsonDocument::fromJson(paramsJsonStr.toUtf8(), &err);
        if (err.error == QJsonParseError::NoError && paramsDoc.isObject()) {
            QJsonObject paramsObj = paramsDoc.object();
            for (auto it = paramsObj.constBegin(); it != paramsObj.constEnd(); ++it) {
                json[it.key()] = it.value(); // 用户参数覆盖默认值
            }
        }
    }

    // 【关键】检查最终 JSON 中 stream 的值
    m_isCurrentStream = json["stream"].toBool();
    qDebug() << "Request mode: Stream =" << m_isCurrentStream;

    // --- 3. 发送请求 ---
    QJsonDocument doc(json);
    QByteArray data = doc.toJson();

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!config.apiKey.isEmpty()) {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(config.apiKey).toUtf8());
    }

    m_currentReply = m_manager->post(request, data);

    // 【关键】如果是流式请求，连接 readyRead 信号
    if (m_isCurrentStream) {
        connect(m_currentReply, &QNetworkReply::readyRead, this, &NetworkManager::onReadyRead);
    }

    connect(m_currentReply, &QNetworkReply::finished, this, &NetworkManager::onReplyFinished);

    int timeoutSec = SettingsManager::instance()->requestTimeout();
    m_timeoutTimer->start(timeoutSec * 1000);
}

// 【新增】处理流式数据
void NetworkManager::onReadyRead()
{
    if (!m_currentReply || !m_isCurrentStream) return;

    // 重置超时计时器
    m_timeoutTimer->start(SettingsManager::instance()->requestTimeout() * 1000);

    // 读取所有可用数据并追加到缓冲区
    m_streamBuffer.append(m_currentReply->readAll());

    // 【关键改进】处理缓冲区中的完整数据块
    // SSE 格式通常以 "data: {...}\n\n" 为单位
    while (true) {
        int newlinePos = m_streamBuffer.indexOf('\n');
        if (newlinePos == -1) {
            // 没有找到换行符，说明数据不完整，等待下一次
            break;
        }

        // 提取一行数据
        QByteArray line = m_streamBuffer.left(newlinePos).trimmed();
        // 从缓冲区移除已处理的数据（包括换行符）
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
                        // OpenAI 格式: choices[0].delta.content
                        QJsonObject delta = choices[0].toObject()["delta"].toObject();
                        if (delta.contains("content")) {
                            QString content = delta["content"].toString();
                            if (!content.isEmpty()) {
                                emit streamDataReceived(content);
                            }
                        }

                        // 【兼容性】有些模型（如 Ollama/OpenAI 兼容）可能没有 delta，直接在 message 里
                        // 仅在非 delta 模式下检查 message (通常流式都有 delta，但保险起见)
                        /*
                         *                       if (delta.isEmpty()) {
                         *                           QJsonObject msg = choices[0].toObject()["message"].toObject();
                         *                           if (msg.contains("content")) {
                         *                               emit streamDataReceived(msg["content"].toString());
                    }
                    }
                    */
                    }
                }
            } else {
                // JSON 解析失败，可能是数据不完整，记录日志
                // qDebug() << "SSE JSON parse error" << err.errorString();
            }
        }
    }
}

void NetworkManager::onReplyFinished()
{
    m_timeoutTimer->stop();
    if (!m_currentReply) return;

    QNetworkReply* reply = m_currentReply;
    m_currentReply = nullptr;

    // 处理网络层错误
    if (reply->error() != QNetworkReply::NoError) {
        QString errorStr = reply->errorString();
        if (reply->error() == QNetworkReply::OperationCanceledError) {
            errorStr = "请求超时或已取消";
        } else {
            // 尝试读取错误详情
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

    // --- 成功处理逻辑 ---

    // 情况 1: 流式请求
    // 流式请求的数据已经通过 readyRead 处理并发送了，finished 只标志着结束
    if (m_isCurrentStream) {
        emit requestFinished(QString(), true, QString());
    }
    // 情况 2: 非流式请求
    // 需要在这里一次性读取所有数据并解析
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
