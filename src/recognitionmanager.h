// // src/recognitionmanager.h

#ifndef RECOGNITIONMANAGER_H
#define RECOGNITIONMANAGER_H

#include <QObject>
#include <QTimer>
#include "networkmanager.h"

class RecognitionManager : public QObject
{
    Q_OBJECT
public:
    explicit RecognitionManager(QObject* parent = nullptr);

    void setAutoUseLastPrompt(bool enabled);
    void recognize(const QString& prompt, const QString& base64Image);
    void onImageChanged(const QString& base64Image);
    void setTempPromptOverride(const QString& prompt);

    // 【新增】提供设置 URL 的接口
    void setServerUrl(const QString& url);
    void setApiKey(const QString& key);     // 【新增】
    void setModelName(const QString& name); // 【新增】

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
    QString m_serverUrl; // 【新增】用于缓存当前的 URL
    QString m_apiKey;      // 【新增】
    QString m_modelName;   // 【新增】
    bool m_isAbortedByUser = false;
};

#endif // RECOGNITIONMANAGER_H
