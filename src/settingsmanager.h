#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QObject>
#include <QSettings>
#include <QString>

class SettingsManager : public QObject
{
    Q_OBJECT
public:
    static SettingsManager* instance();

    // 【新增】显式同步保存到磁盘
    void sync();

    // 服务器设置
    QString serverUrl() const;
    void setServerUrl(const QString& url);

    // 快捷键设置
    QString screenshotShortcut() const;
    QString textRecognizeShortcut() const;
    QString formulaRecognizeShortcut() const;
    QString tableRecognizeShortcut() const;

    void setScreenshotShortcut(const QString& key);
    void setTextRecognizeShortcut(const QString& key);
    void setFormulaRecognizeShortcut(const QString& key);
    void setTableRecognizeShortcut(const QString& key);

    // 行为设置
    bool autoUseLastPrompt() const;
    void setAutoUseLastPrompt(bool enabled);

    QString displayMathEnvironment() const;
    void setDisplayMathEnvironment(const QString& env);

    // 【新增】数学字体设置
    QString mathFont() const;
    void setMathFont(const QString& font);

    // 外部程序设置
    QString externalProcessorCommand() const;
    void setExternalProcessorCommand(const QString& cmd);

    bool autoCopyResult() const;
    void setAutoCopyResult(bool enabled);

    bool autoRecognizeOnScreenshot() const;
    void setAutoRecognizeOnScreenshot(bool enabled);

    // 服务管理设置
    bool autoStartService() const;
    void setAutoStartService(bool enabled);

    QString serviceStartCommand() const;
    void setServiceStartCommand(const QString& cmd);

    int serviceIdleTimeout() const;
    void setServiceIdleTimeout(int minutes);

    // 【新增】请求参数配置
    QString requestParameters() const;
    void setRequestParameters(const QString& json);

    // 【新增】图片预览模式
    int imageViewMode() const;
    void setImageViewMode(int mode);


    // 提示词设置
    QString textPrompt() const;
    QString formulaPrompt() const;
    QString tablePrompt() const;

    void setTextPrompt(const QString& prompt);
    void setFormulaPrompt(const QString& prompt);
    void setTablePrompt(const QString& prompt);

signals:
    void serverUrlChanged(const QString& url);
    void shortcutsChanged();
    void autoUseLastPromptChanged(bool enabled);
    void displayMathEnvironmentChanged(const QString& env);
    void mathFontChanged(const QString& font); // 【新增】
    void externalProcessorCommandChanged(const QString& cmd);

    void autoCopyResultChanged(bool enabled);
    void autoRecognizeOnScreenshotChanged(bool enabled);

    void autoStartServiceChanged(bool enabled);
    void serviceStartCommandChanged(const QString& cmd);
    void serviceIdleTimeoutChanged(int minutes);

    void requestParametersChanged(const QString& json);

    void imageViewModeChanged(int mode);

    void textPromptChanged(const QString& prompt);
    void formulaPromptChanged(const QString& prompt);
    void tablePromptChanged(const QString& prompt);

private:
    explicit SettingsManager(QObject* parent = nullptr);

    // 【新增】初始化默认值逻辑
    void initializeDefaults();

    QSettings m_settings;
};

#endif // SETTINGSMANAGER_H
