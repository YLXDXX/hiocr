#include "settingsmanager.h"
#include "constants.h" // 引入常量

SettingsManager* SettingsManager::instance()
{
    static SettingsManager* s_instance = nullptr;
    if (!s_instance) s_instance = new SettingsManager();
    return s_instance;
}

SettingsManager::SettingsManager(QObject* parent) : QObject(parent)
{
}

QString SettingsManager::serverUrl() const {
    // 使用 Constants 命名空间
    return m_settings.value("server/url", Constants::DEFAULT_SERVER_URL).toString();
}

void SettingsManager::setServerUrl(const QString& url) {
    if (serverUrl() != url) {
        m_settings.setValue("server/url", url);
        emit serverUrlChanged(url);
    }
}

QString SettingsManager::screenshotShortcut() const {
    return m_settings.value("shortcuts/screenshot", Constants::SHORTCUT_SCREENSHOT).toString();
}

QString SettingsManager::textRecognizeShortcut() const {
    return m_settings.value("shortcuts/text_recognize", Constants::SHORTCUT_TEXT).toString();
}

QString SettingsManager::formulaRecognizeShortcut() const {
    return m_settings.value("shortcuts/formula_recognize", Constants::SHORTCUT_FORMULA).toString();
}

QString SettingsManager::tableRecognizeShortcut() const {
    return m_settings.value("shortcuts/table_recognize", Constants::SHORTCUT_TABLE).toString();
}

// Setters 保持不变
void SettingsManager::setScreenshotShortcut(const QString& key) {
    m_settings.setValue("shortcuts/screenshot", key);
    emit shortcutsChanged();
}

void SettingsManager::setTextRecognizeShortcut(const QString& key) {
    m_settings.setValue("shortcuts/text_recognize", key);
    emit shortcutsChanged();
}

void SettingsManager::setFormulaRecognizeShortcut(const QString& key) {
    m_settings.setValue("shortcuts/formula_recognize", key);
    emit shortcutsChanged();
}

void SettingsManager::setTableRecognizeShortcut(const QString& key) {
    m_settings.setValue("shortcuts/table_recognize", key);
    emit shortcutsChanged();
}

bool SettingsManager::autoUseLastPrompt() const {
    return m_settings.value("auto_use_last_prompt", true).toBool();
}

void SettingsManager::setAutoUseLastPrompt(bool enabled) {
    if (autoUseLastPrompt() != enabled) {
        m_settings.setValue("auto_use_last_prompt", enabled);
        emit autoUseLastPromptChanged(enabled);
    }
}
