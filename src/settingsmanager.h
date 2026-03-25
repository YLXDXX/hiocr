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

signals:
    void serverUrlChanged(const QString& url);
    void shortcutsChanged();
    void autoUseLastPromptChanged(bool enabled);
    void displayMathEnvironmentChanged(const QString& env);
    void mathFontChanged(const QString& font); // 【新增】
    void externalProcessorCommandChanged(const QString& cmd);

    void autoCopyResultChanged(bool enabled);
    void autoRecognizeOnScreenshotChanged(bool enabled);

private:
    explicit SettingsManager(QObject* parent = nullptr);
    QSettings m_settings;
};

#endif // SETTINGSMANAGER_H
