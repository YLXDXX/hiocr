#include "recognitionmanager.h"
#include "settingsmanager.h"
#include "networkmanager.h"

RecognitionManager::RecognitionManager(QObject* parent) : QObject(parent)
{
    m_networkManager = new NetworkManager(this);
    connect(m_networkManager, &NetworkManager::requestFinished, this, &RecognitionManager::onNetworkFinished);

    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    connect(m_debounceTimer, &QTimer::timeout, this, &RecognitionManager::onDebounceTimeout);

    m_lastPrompt = "文字识别:";
}

void RecognitionManager::recognize(const QString& prompt, const QString& base64Image) {
    if (base64Image.isEmpty()) return;

    m_lastPrompt = prompt;
    m_isBusy = true;
    emit busyStateChanged(true);

    // 构建请求配置
    RequestConfig config;
    config.base64Image = base64Image;
    config.prompt = prompt;
    config.serverUrl = m_serverUrl; // 使用内部缓存的 URL

    // 发送请求
    m_networkManager->sendRequest(config);
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

    if (success) {
        emit recognitionFinished(result);
    } else {
        emit recognitionFailed(error);
    }
}

// 【新增】实现 setServerUrl
void RecognitionManager::setServerUrl(const QString& url)
{
    if (m_networkManager) {
        // NetworkManager 不再需要单独设置 URL，通过 config 传入
        // 但为了保持缓存，我们在 Manager 里存一份
        m_serverUrl = url;
    }
}
