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

signals:
    void serverUrlChanged(const QString& url);
    void shortcutsChanged();
    void autoUseLastPromptChanged(bool enabled);

private:
    explicit SettingsManager(QObject* parent = nullptr);
    QSettings m_settings;
};

#endif // SETTINGSMANAGER_H
