#include "settingsmanager.h"
#include "constants.h" // 引入常量

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
    // 初始化默认配置（如果配置文件不存在）
    initializeDefaults();
}


// 【新增】实现
QString SettingsManager::requestParameters() const {
    // 如果用户未设置，返回默认 JSON
    return m_settings.value("network/request_parameters", Constants::DEFAULT_REQUEST_PARAMETERS).toString();
}

void SettingsManager::setRequestParameters(const QString& json) {
    if (requestParameters() != json) {
        m_settings.setValue("network/request_parameters", json);
        emit requestParametersChanged(json);
    }
}

// 【新增】检查并初始化默认配置文件
void SettingsManager::initializeDefaults()
{
    // 检测配置文件是否存在
    // QSettings 默认存储路径在 Linux 下通常为 ~/.config/hiocr/hiocr.conf
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/hiocr/hiocr.conf";

    // QFile::exists 不支持检测 QSettings 的 "UserScope" 抽象路径，因此我们直接尝试读取一个关键键值
    // 如果 server/url 键不存在，说明这是一个全新的配置环境
    if (!m_settings.contains("server/url")) {
        qDebug() << "Configuration file not found or empty, initializing with defaults...";

        // 批量写入所有默认值
        m_settings.setValue("server/url", Constants::DEFAULT_SERVER_URL);

        m_settings.setValue("shortcuts/screenshot", Constants::SHORTCUT_SCREENSHOT);
        m_settings.setValue("shortcuts/text_recognize", Constants::SHORTCUT_TEXT);
        m_settings.setValue("shortcuts/formula_recognize", Constants::SHORTCUT_FORMULA);
        m_settings.setValue("shortcuts/table_recognize", Constants::SHORTCUT_TABLE);

        m_settings.setValue("auto_use_last_prompt", true);
        m_settings.setValue("display_math_env", Constants::DEFAULT_DISPLAY_MATH_ENV);
        m_settings.setValue("display/math_font", Constants::DEFAULT_MATH_FONT);

        m_settings.setValue("external_processor/command", Constants::DEFAULT_EXTERNAL_PROCESSOR);
        m_settings.setValue("behavior/auto_copy_result", Constants::DEFAULT_AUTO_COPY_RESULT);
        m_settings.setValue("behavior/auto_recognize_screenshot", Constants::DEFAULT_AUTO_RECOGNIZE_SCREENSHOT);

        m_settings.setValue("service/auto_start", Constants::DEFAULT_AUTO_START_SERVICE);
        m_settings.setValue("service/start_command", Constants::DEFAULT_SERVICE_START_COMMAND);
        m_settings.setValue("service/idle_timeout", Constants::DEFAULT_SERVICE_IDLE_TIMEOUT);

        // 强制写入磁盘
        m_settings.sync();
        qDebug() << "Configuration file created at:" << m_settings.fileName();
    }

    // 【新增】
    if (!m_settings.contains("network/request_parameters")) {
        m_settings.setValue("network/request_parameters", Constants::DEFAULT_REQUEST_PARAMETERS);
    }
}

// 【新增】提供显式保存接口
void SettingsManager::sync()
{
    m_settings.sync();
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

// 【新增】实现
QString SettingsManager::mathFont() const {
    return m_settings.value("display/math_font", Constants::DEFAULT_MATH_FONT).toString();
}

void SettingsManager::setMathFont(const QString& font) {
    if (mathFont() != font) {
        m_settings.setValue("display/math_font", font);
        emit mathFontChanged(font);
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

