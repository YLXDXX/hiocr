#include "recognitionmanager.h"
#include "settingsmanager.h"
#include "networkmanager.h"

RecognitionManager::RecognitionManager(QObject* parent) : QObject(parent)
{
    m_networkManager = new NetworkManager(this);
    connect(m_networkManager, &NetworkManager::requestFinished, this, &RecognitionManager::onNetworkFinished);
    connect(m_networkManager, &NetworkManager::streamDataReceived, this, &RecognitionManager::streamDataReceived);

    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    connect(m_debounceTimer, &QTimer::timeout, this, &RecognitionManager::onDebounceTimeout);

    m_lastPrompt = "文字识别:";
}

void RecognitionManager::recognize(const QString& prompt, const QString& base64Image) {
    if (base64Image.isEmpty()) return;

    m_isAbortedByUser = false;  // 【新增】每次新请求重置中断标志

    m_lastPrompt = prompt;
    m_isBusy = true;
    emit busyStateChanged(true);

    RequestConfig config;
    config.base64Image = base64Image;
    config.prompt = prompt;
    config.serverUrl = m_serverUrl;
    config.apiKey = m_apiKey;
    config.model = m_modelName;

    m_networkManager->sendRequest(config);
}

void RecognitionManager::abortRecognition()
{
    if (!m_isBusy) return;  // 不在识别中则忽略
    m_isAbortedByUser = true;
    m_networkManager->abortRequest();
}

void RecognitionManager::onImageChanged(const QString& base64Image) {
    m_currentBase64 = base64Image;
    if (m_currentBase64.isEmpty()) return;

    // 防抖：300ms内如果图片再次变化，重新计时
    m_debounceTimer->start(300);
}

void RecognitionManager::setTempPromptOverride(const QString& prompt) {
    m_tempPromptOverride = prompt;
}

QString RecognitionManager::lastPrompt() const {
    return m_lastPrompt;
}

void RecognitionManager::setLastPrompt(const QString& prompt) {
    m_lastPrompt = prompt;
}

void RecognitionManager::setAutoUseLastPrompt(bool enabled) {
    m_autoUseLastPrompt = enabled;
}

void RecognitionManager::onDebounceTimeout() {
    if (m_currentBase64.isEmpty()) return;

    QString promptToUse;
    if (!m_tempPromptOverride.isEmpty()) {
        promptToUse = m_tempPromptOverride;
        m_tempPromptOverride.clear(); // 仅使用一次
    } else {
        promptToUse = m_autoUseLastPrompt ? m_lastPrompt : "文字识别:";
    }

    recognize(promptToUse, m_currentBase64);
}

void RecognitionManager::onNetworkFinished(const QString& result, bool success, const QString& error) {
    m_isBusy = false;
    emit busyStateChanged(false);

    // 【新增】用户主动中断，不触发失败重试逻辑
    if (m_isAbortedByUser) {
        m_isAbortedByUser = false;
        emit recognitionAborted();
        return;
    }

    if (success) {
        emit recognitionFinished(result);
    } else {
        emit recognitionFailed(error);
    }
}

void RecognitionManager::setServerUrl(const QString& url) { m_serverUrl = url; }
void RecognitionManager::setApiKey(const QString& key) { m_apiKey = key; }
void RecognitionManager::setModelName(const QString& name) { m_modelName = name; }
