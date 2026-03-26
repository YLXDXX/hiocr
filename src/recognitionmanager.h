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

    QString lastPrompt() const;
    void setLastPrompt(const QString& prompt);

    // 获取当前图片 Base64
    QString currentBase64() const { return m_currentBase64; }

    // 【新增】设置当前图片 Base64（用于静态展示，不触发识别）
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
