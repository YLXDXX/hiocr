#include "settingsmanager.h"
#include "constants.h"

#include <QFile>
#include <QStandardPaths>
#include <QDebug>

SettingsManager* SettingsManager::instance()
{
    static SettingsManager* s_instance = nullptr;
    if (!s_instance) s_instance = new SettingsManager();
    return s_instance;
}

SettingsManager::SettingsManager(QObject* parent) : QObject(parent)
{
    // 初始化默认配置（如果配置文件不存在或版本过旧）
    initializeDefaults();
}

void SettingsManager::initializeDefaults()
{
    // 检测配置文件是否存在关键键值，以此判断是否为全新环境
    if (!m_settings.contains("server/url")) {
        qDebug() << "Configuration file not found or empty, initializing with defaults...";

        // 批量写入所有默认值
        m_settings.setValue("server/url", Constants::DEFAULT_SERVER_URL);

        m_settings.setValue("shortcuts/screenshot", Constants::SHORTCUT_SCREENSHOT);
        m_settings.setValue("shortcuts/text_recognize", Constants::SHORTCUT_TEXT);
        m_settings.setValue("shortcuts/formula_recognize", Constants::SHORTCUT_FORMULA);
        m_settings.setValue("shortcuts/table_recognize", Constants::SHORTCUT_TABLE);
        m_settings.setValue("shortcuts/external_process", Constants::SHORTCUT_EXTERNAL_PROCESS);

        m_settings.setValue("behavior/auto_use_last_prompt", true);
        m_settings.setValue("display_math_env", Constants::DEFAULT_DISPLAY_MATH_ENV);
        m_settings.setValue("display/math_font", Constants::DEFAULT_MATH_FONT);

        m_settings.setValue("external_processor/command", Constants::DEFAULT_EXTERNAL_PROCESSOR);
        m_settings.setValue("behavior/auto_copy_result", Constants::DEFAULT_AUTO_COPY_RESULT);
        m_settings.setValue("behavior/auto_recognize_screenshot", Constants::DEFAULT_AUTO_RECOGNIZE_SCREENSHOT);

        // 【修复】写入缺失的默认设置
        m_settings.setValue("behavior/auto_external_process_before_copy", Constants::DEFAULT_AUTO_EXTERNAL_PROCESS_BEFORE_COPY);

        m_settings.setValue("service/auto_start", Constants::DEFAULT_AUTO_START_SERVICE);
        m_settings.setValue("service/start_command", Constants::DEFAULT_SERVICE_START_COMMAND);
        m_settings.setValue("service/idle_timeout", Constants::DEFAULT_SERVICE_IDLE_TIMEOUT);

        m_settings.setValue("network/request_parameters", Constants::DEFAULT_REQUEST_PARAMETERS);
        m_settings.setValue("display/view_mode", Constants::DEFAULT_IMAGE_VIEW_MODE);

        m_settings.setValue("prompts/text", Constants::PROMPT_TEXT);
        m_settings.setValue("prompts/formula", Constants::PROMPT_FORMULA);
        m_settings.setValue("prompts/table", Constants::PROMPT_TABLE);

        // 强制写入磁盘
        m_settings.sync();
        qDebug() << "Configuration file created at:" << m_settings.fileName();
    }
    else {
        // 【修复】兼容性检查：如果配置文件存在但缺少某些键（例如更新软件后），补充默认值
        if (!m_settings.contains("behavior/auto_external_process_before_copy")) {
            m_settings.setValue("behavior/auto_external_process_before_copy", Constants::DEFAULT_AUTO_EXTERNAL_PROCESS_BEFORE_COPY);
        }
        if (!m_settings.contains("shortcuts/external_process")) {
            m_settings.setValue("shortcuts/external_process", Constants::SHORTCUT_EXTERNAL_PROCESS);
        }
        if (!m_settings.contains("network/request_parameters")) {
            m_settings.setValue("network/request_parameters", Constants::DEFAULT_REQUEST_PARAMETERS);
        }
        if (!m_settings.contains("display/math_font")) {
            m_settings.setValue("display/math_font", Constants::DEFAULT_MATH_FONT);
        }
        if (!m_settings.contains("display/view_mode")) {
            m_settings.setValue("display/view_mode", Constants::DEFAULT_IMAGE_VIEW_MODE);
        }
        if (!m_settings.contains("prompts/text")) {
            m_settings.setValue("prompts/text", Constants::PROMPT_TEXT);
        }
        if (!m_settings.contains("prompts/formula")) {
            m_settings.setValue("prompts/formula", Constants::PROMPT_FORMULA);
        }
        if (!m_settings.contains("prompts/table")) {
            m_settings.setValue("prompts/table", Constants::PROMPT_TABLE);
        }
        if (!m_settings.contains("shortcuts/external_process")) {
            m_settings.setValue("shortcuts/external_process", Constants::SHORTCUT_EXTERNAL_PROCESS);
        }
    }
}

void SettingsManager::sync()
{
    m_settings.sync();
}

// --- 服务器设置 ---

QString SettingsManager::serverUrl() const {
    return m_settings.value("server/url", Constants::DEFAULT_SERVER_URL).toString();
}

void SettingsManager::setServerUrl(const QString& url) {
    if (serverUrl() != url) {
        m_settings.setValue("server/url", url);
        emit serverUrlChanged(url);
    }
}

// --- 数学字体设置 ---

QString SettingsManager::mathFont() const {
    return m_settings.value("display/math_font", Constants::DEFAULT_MATH_FONT).toString();
}

void SettingsManager::setMathFont(const QString& font) {
    if (mathFont() != font) {
        m_settings.setValue("display/math_font", font);
        emit mathFontChanged(font);
    }
}

// --- 快捷键设置 ---

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

QString SettingsManager::externalProcessShortcut() const {
    return m_settings.value("shortcuts/external_process", Constants::SHORTCUT_EXTERNAL_PROCESS).toString();
}

void SettingsManager::setScreenshotShortcut(const QString& key) {
    if (screenshotShortcut() != key) {
        m_settings.setValue("shortcuts/screenshot", key);
        emit shortcutsChanged();
    }
}

void SettingsManager::setTextRecognizeShortcut(const QString& key) {
    if (textRecognizeShortcut() != key) {
        m_settings.setValue("shortcuts/text_recognize", key);
        emit shortcutsChanged();
    }
}

void SettingsManager::setFormulaRecognizeShortcut(const QString& key) {
    if (formulaRecognizeShortcut() != key) {
        m_settings.setValue("shortcuts/formula_recognize", key);
        emit shortcutsChanged();
    }
}

void SettingsManager::setTableRecognizeShortcut(const QString& key) {
    if (tableRecognizeShortcut() != key) {
        m_settings.setValue("shortcuts/table_recognize", key);
        emit shortcutsChanged();
    }
}

void SettingsManager::setExternalProcessShortcut(const QString& key) {
    if (externalProcessShortcut() != key) {
        m_settings.setValue("shortcuts/external_process", key);
        emit externalProcessShortcutChanged(key);
    }
}

// --- 行为设置 ---

bool SettingsManager::autoUseLastPrompt() const {
    return m_settings.value("behavior/auto_use_last_prompt", true).toBool();
}

void SettingsManager::setAutoUseLastPrompt(bool enabled) {
    if (autoUseLastPrompt() != enabled) {
        m_settings.setValue("behavior/auto_use_last_prompt", enabled);
        emit autoUseLastPromptChanged(enabled);
    }
}

QString SettingsManager::displayMathEnvironment() const {
    return m_settings.value("display_math_env", Constants::DEFAULT_DISPLAY_MATH_ENV).toString();
}

void SettingsManager::setDisplayMathEnvironment(const QString& env) {
    if (displayMathEnvironment() != env) {
        m_settings.setValue("display_math_env", env);
        emit displayMathEnvironmentChanged(env);
    }
}

QString SettingsManager::externalProcessorCommand() const {
    return m_settings.value("external_processor/command", Constants::DEFAULT_EXTERNAL_PROCESSOR).toString();
}

void SettingsManager::setExternalProcessorCommand(const QString& cmd) {
    if (externalProcessorCommand() != cmd) {
        m_settings.setValue("external_processor/command", cmd);
        emit externalProcessorCommandChanged(cmd);
    }
}

bool SettingsManager::autoCopyResult() const {
    return m_settings.value("behavior/auto_copy_result", Constants::DEFAULT_AUTO_COPY_RESULT).toBool();
}

void SettingsManager::setAutoCopyResult(bool enabled) {
    if (autoCopyResult() != enabled) {
        m_settings.setValue("behavior/auto_copy_result", enabled);
        emit autoCopyResultChanged(enabled);
    }
}

bool SettingsManager::autoRecognizeOnScreenshot() const {
    return m_settings.value("behavior/auto_recognize_screenshot", Constants::DEFAULT_AUTO_RECOGNIZE_SCREENSHOT).toBool();
}

void SettingsManager::setAutoRecognizeOnScreenshot(bool enabled) {
    if (autoRecognizeOnScreenshot() != enabled) {
        m_settings.setValue("behavior/auto_recognize_screenshot", enabled);
        emit autoRecognizeOnScreenshotChanged(enabled);
    }
}

// 【修复】实现复制前自动处理设置
bool SettingsManager::autoExternalProcessBeforeCopy() const {
    return m_settings.value("behavior/auto_external_process_before_copy", Constants::DEFAULT_AUTO_EXTERNAL_PROCESS_BEFORE_COPY).toBool();
}

void SettingsManager::setAutoExternalProcessBeforeCopy(bool enabled) {
    if (autoExternalProcessBeforeCopy() != enabled) {
        m_settings.setValue("behavior/auto_external_process_before_copy", enabled);
        emit autoExternalProcessBeforeCopyChanged(enabled);
    }
}

// --- 服务管理设置 ---

bool SettingsManager::autoStartService() const {
    return m_settings.value("service/auto_start", Constants::DEFAULT_AUTO_START_SERVICE).toBool();
}

void SettingsManager::setAutoStartService(bool enabled) {
    if (autoStartService() != enabled) {
        m_settings.setValue("service/auto_start", enabled);
        emit autoStartServiceChanged(enabled);
    }
}

QString SettingsManager::serviceStartCommand() const {
    return m_settings.value("service/start_command", Constants::DEFAULT_SERVICE_START_COMMAND).toString();
}

void SettingsManager::setServiceStartCommand(const QString& cmd) {
    if (serviceStartCommand() != cmd) {
        m_settings.setValue("service/start_command", cmd);
        emit serviceStartCommandChanged(cmd);
    }
}

int SettingsManager::serviceIdleTimeout() const {
    return m_settings.value("service/idle_timeout", Constants::DEFAULT_SERVICE_IDLE_TIMEOUT).toInt();
}

void SettingsManager::setServiceIdleTimeout(int minutes) {
    if (serviceIdleTimeout() != minutes) {
        m_settings.setValue("service/idle_timeout", minutes);
        emit serviceIdleTimeoutChanged(minutes);
    }
}

// --- 网络请求设置 ---

QString SettingsManager::requestParameters() const {
    return m_settings.value("network/request_parameters", Constants::DEFAULT_REQUEST_PARAMETERS).toString();
}

void SettingsManager::setRequestParameters(const QString& json) {
    if (requestParameters() != json) {
        m_settings.setValue("network/request_parameters", json);
        emit requestParametersChanged(json);
    }
}

// --- 界面设置 ---

int SettingsManager::imageViewMode() const {
    return m_settings.value("display/view_mode", Constants::DEFAULT_IMAGE_VIEW_MODE).toInt();
}

void SettingsManager::setImageViewMode(int mode) {
    if (imageViewMode() != mode) {
        m_settings.setValue("display/view_mode", mode);
        emit imageViewModeChanged(mode);
    }
}

// --- 提示词设置 ---

QString SettingsManager::textPrompt() const {
    return m_settings.value("prompts/text", Constants::PROMPT_TEXT).toString();
}

void SettingsManager::setTextPrompt(const QString& prompt) {
    if (textPrompt() != prompt) {
        m_settings.setValue("prompts/text", prompt);
        emit textPromptChanged(prompt);
    }
}

QString SettingsManager::formulaPrompt() const {
    return m_settings.value("prompts/formula", Constants::PROMPT_FORMULA).toString();
}

void SettingsManager::setFormulaPrompt(const QString& prompt) {
    if (formulaPrompt() != prompt) {
        m_settings.setValue("prompts/formula", prompt);
        emit formulaPromptChanged(prompt);
    }
}

QString SettingsManager::tablePrompt() const {
    return m_settings.value("prompts/table", Constants::PROMPT_TABLE).toString();
}

void SettingsManager::setTablePrompt(const QString& prompt) {
    if (tablePrompt() != prompt) {
        m_settings.setValue("prompts/table", prompt);
        emit tablePromptChanged(prompt);
    }
}
