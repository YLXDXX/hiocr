// // src/recognitionmanager.h

#ifndef RECOGNITIONMANAGER_H
#define RECOGNITIONMANAGER_H

#include <QObject>
#include <QTimer>
#include "networkmanager.h"

#include "settingsmanager.h"

class RecognitionManager : public QObject
{
    Q_OBJECT
public:
    explicit RecognitionManager(QObject* parent = nullptr);

    void setAutoUseLastPrompt(bool enabled);
    void recognize(const QString& prompt, const QString& base64Image);
    void onImageChanged(const QString& base64Image);
    void setTempPromptOverride(const QString& prompt);

    void setServerUrl(const QString& url);
    void setApiKey(const QString& key);
    void setModelName(const QString& name);

    QString lastPrompt() const;
    void setLastPrompt(const QString& prompt);

    QString currentBase64() const { return m_currentBase64; }
    void setCurrentBase64(const QString& base64) { m_currentBase64 = base64; }

    void abortRecognition();

signals:
    void recognitionFinished(const QString& markdown);
    void recognitionFailed(const QString& error);
    void busyStateChanged(bool busy);
    void streamDataReceived(const QString& delta);
    void recognitionAborted();

private slots:
    void onNetworkFinished(const QString& result, bool success, const QString& error);
    void onDebounceTimeout();

private:
    NetworkManager* m_networkManager;
    QTimer* m_debounceTimer;
    QString m_lastPrompt;
    QString m_currentBase64;
    QString m_tempPromptOverride;
    bool m_autoUseLastPrompt = true;
    bool m_isBusy = false;
    QString m_serverUrl;
    QString m_apiKey;
    QString m_modelName;
    bool m_isAbortedByUser = false;
};

// --- Implementation ---

inline RecognitionManager::RecognitionManager(QObject* parent) : QObject(parent)
{
    m_networkManager = new NetworkManager(this);
    connect(m_networkManager, &NetworkManager::requestFinished, this, &RecognitionManager::onNetworkFinished);
    connect(m_networkManager, &NetworkManager::streamDataReceived, this, &RecognitionManager::streamDataReceived);

    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    connect(m_debounceTimer, &QTimer::timeout, this, &RecognitionManager::onDebounceTimeout);

    m_lastPrompt = "文字识别:";
}

inline void RecognitionManager::recognize(const QString& prompt, const QString& base64Image) {
    if (base64Image.isEmpty()) return;

    m_isAbortedByUser = false;

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

inline void RecognitionManager::abortRecognition()
{
    if (!m_isBusy) return;
    m_isAbortedByUser = true;
    m_networkManager->abortRequest();
}

inline void RecognitionManager::onImageChanged(const QString& base64Image) {
    m_currentBase64 = base64Image;
    if (m_currentBase64.isEmpty()) return;

    m_debounceTimer->start(300);
}

inline void RecognitionManager::setTempPromptOverride(const QString& prompt) {
    m_tempPromptOverride = prompt;
}

inline QString RecognitionManager::lastPrompt() const {
    return m_lastPrompt;
}

inline void RecognitionManager::setLastPrompt(const QString& prompt) {
    m_lastPrompt = prompt;
}

inline void RecognitionManager::setAutoUseLastPrompt(bool enabled) {
    m_autoUseLastPrompt = enabled;
}

inline void RecognitionManager::onDebounceTimeout() {
    if (m_currentBase64.isEmpty()) return;

    QString promptToUse;
    if (!m_tempPromptOverride.isEmpty()) {
        promptToUse = m_tempPromptOverride;
        m_tempPromptOverride.clear();
    } else {
        promptToUse = m_autoUseLastPrompt ? m_lastPrompt : "文字识别:";
    }

    recognize(promptToUse, m_currentBase64);
}

inline void RecognitionManager::onNetworkFinished(const QString& result, bool success, const QString& error) {
    m_isBusy = false;
    emit busyStateChanged(false);

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

inline void RecognitionManager::setServerUrl(const QString& url) { m_serverUrl = url; }
inline void RecognitionManager::setApiKey(const QString& key) { m_apiKey = key; }
inline void RecognitionManager::setModelName(const QString& name) { m_modelName = name; }

#endif // RECOGNITIONMANAGER_H
