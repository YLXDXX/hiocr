// src/recognitionmanager.h

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

    QString lastPrompt() const;
    void setLastPrompt(const QString& prompt);

    QString currentBase64() const { return m_currentBase64; }
    void setCurrentBase64(const QString& base64) { m_currentBase64 = base64; }

signals:
    void recognitionFinished(const QString& markdown);
    void recognitionFailed(const QString& error);
    void busyStateChanged(bool busy);

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
};

#endif // RECOGNITIONMANAGER_H
