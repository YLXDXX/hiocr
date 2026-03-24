#include "settingsmanager.h"

const QString SettingsManager::DefaultServerUrl = "http://localhost:8080/v1/chat/completions";
const QString SettingsManager::DefaultScreenshotShortcut = "Ctrl+Shift+S";
const QString SettingsManager::DefaultTextShortcut = "Ctrl+Shift+T";
const QString SettingsManager::DefaultFormulaShortcut = "Ctrl+Shift+F";
const QString SettingsManager::DefaultTableShortcut = "Ctrl+Shift+B";

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
    return m_settings.value("server/url", DefaultServerUrl).toString();
}

void SettingsManager::setServerUrl(const QString& url) {
    if (serverUrl() != url) {
        m_settings.setValue("server/url", url);
        emit serverUrlChanged(url);
    }
}

QString SettingsManager::screenshotShortcut() const {
    return m_settings.value("shortcuts/screenshot", DefaultScreenshotShortcut).toString();
}

QString SettingsManager::textRecognizeShortcut() const {
    return m_settings.value("shortcuts/text_recognize", DefaultTextShortcut).toString();
}

QString SettingsManager::formulaRecognizeShortcut() const {
    return m_settings.value("shortcuts/formula_recognize", DefaultFormulaShortcut).toString();
}

QString SettingsManager::tableRecognizeShortcut() const {
    return m_settings.value("shortcuts/table_recognize", DefaultTableShortcut).toString();
}

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
