#ifndef SETTINGSMANAGER_IMPL_H
#define SETTINGSMANAGER_IMPL_H

#include "settingsmanager.h"

// ====== Method implementations ======

#include <QAtomicPointer>

inline SettingsManager* SettingsManager::instance()
{
    static QAtomicPointer<SettingsManager> s_instance;
    if (!s_instance.loadAcquire()) {
        SettingsManager* inst = new SettingsManager();
        if (!s_instance.testAndSetRelease(nullptr, inst)) {
            delete inst;
        }
    }
    return s_instance.loadAcquire();
}

inline SettingsManager::SettingsManager(QObject* parent) : QObject(parent)
{
    initializeDefaults();
}

inline void SettingsManager::initializeDefaults()
{
    // 1. 如果是全新安装（不存在主键）
    if (!m_settings.contains("server/url")) {
        qDebug() << "Initializing full defaults...";

        m_settings.setValue("server/url", Constants::DEFAULT_SERVER_URL);
        m_settings.setValue("server/api_key", Constants::DEFAULT_GLOBAL_API_KEY);
        m_settings.setValue("server/model_name", Constants::DEFAULT_GLOBAL_MODEL_NAME);
        m_settings.setValue("history/save_enabled", Constants::DEFAULT_SAVE_HISTORY);
        m_settings.setValue("history/limit", Constants::DEFAULT_HISTORY_LIMIT);
        m_settings.setValue("behavior/silent_mode_enabled", Constants::DEFAULT_SILENT_MODE_ENABLED);
        m_settings.setValue("behavior/silent_mode_notification_type", Constants::DEFAULT_SILENT_MODE_NOTIFICATION_TYPE);

        m_settings.setValue("floating_ball/size", Constants::DEFAULT_FLOATING_BALL_SIZE);
        m_settings.setValue("floating_ball/pos_x", Constants::DEFAULT_FLOATING_BALL_POS_X);
        m_settings.setValue("floating_ball/pos_y", Constants::DEFAULT_FLOATING_BALL_POS_Y);
        m_settings.setValue("floating_ball/auto_hide_time", Constants::DEFAULT_FLOATING_BALL_AUTO_HIDE_TIME);
        m_settings.setValue("floating_ball/always_visible", Constants::DEFAULT_FLOATING_BALL_ALWAYS_VISIBLE);

        m_settings.setValue("shortcuts/screenshot", Constants::SHORTCUT_SCREENSHOT);
        m_settings.setValue("shortcuts/text_recognize", Constants::SHORTCUT_TEXT);
        m_settings.setValue("shortcuts/formula_recognize", Constants::SHORTCUT_FORMULA);
        m_settings.setValue("shortcuts/table_recognize", Constants::SHORTCUT_TABLE);
        m_settings.setValue("shortcuts/abort", Constants::SHORTCUT_ABORT);
        m_settings.setValue("shortcuts/external_process", Constants::SHORTCUT_EXTERNAL_PROCESS);
        m_settings.setValue("behavior/auto_use_last_prompt", true);
        m_settings.setValue("display_math_env", Constants::DEFAULT_DISPLAY_MATH_ENV);
        m_settings.setValue("display/math_font", Constants::DEFAULT_MATH_FONT);
        m_settings.setValue("external_processor/command", Constants::DEFAULT_EXTERNAL_PROCESSOR);
        m_settings.setValue("behavior/auto_copy_result", Constants::DEFAULT_AUTO_COPY_RESULT);
        m_settings.setValue("behavior/auto_recognize_screenshot", Constants::DEFAULT_AUTO_RECOGNIZE_SCREENSHOT);
        m_settings.setValue("behavior/auto_external_process_before_copy", Constants::DEFAULT_AUTO_EXTERNAL_PROCESS_BEFORE_COPY);
        m_settings.setValue("service/auto_start", Constants::DEFAULT_AUTO_START_SERVICE);
        m_settings.setValue("service/start_command", Constants::DEFAULT_SERVICE_START_COMMAND);
        m_settings.setValue("service/idle_timeout", Constants::DEFAULT_SERVICE_IDLE_TIMEOUT);
        m_settings.setValue("network/request_parameters", Constants::DEFAULT_REQUEST_PARAMETERS);
        m_settings.setValue("network/request_timeout", Constants::DEFAULT_REQUEST_TIMEOUT);
        m_settings.setValue("display/view_mode", Constants::DEFAULT_IMAGE_VIEW_MODE);
        m_settings.setValue("prompts/text", Constants::PROMPT_TEXT);
        m_settings.setValue("prompts/formula", Constants::PROMPT_FORMULA);
        m_settings.setValue("prompts/table", Constants::PROMPT_TABLE);
        m_settings.setValue("display/renderer_font_size", Constants::DEFAULT_RENDERER_FONT_SIZE);
        m_settings.setValue("display/source_editor_font_size", Constants::DEFAULT_SOURCE_EDITOR_FONT_SIZE);

        // 初始化默认服务列表
        QList<ServiceProfile> defaults;
        ServiceProfile s1;
        s1.id = QUuid::createUuid().toString();
        s1.name = Constants::DEFAULT_SERVICE_1_NAME;
        s1.startCommand = Constants::DEFAULT_SERVICE_1_CMD;
        s1.serverUrl = Constants::DEFAULT_SERVICE_1_URL;
        s1.textPrompt = Constants::PROMPT_TEXT;
        s1.formulaPrompt = Constants::PROMPT_FORMULA;
        s1.tablePrompt = Constants::PROMPT_TABLE;
        defaults.append(s1);

        ServiceProfile s2;
        s2.id = QUuid::createUuid().toString();
        s2.name = Constants::DEFAULT_SERVICE_2_NAME;
        s2.startCommand = Constants::DEFAULT_SERVICE_2_CMD;
        s2.serverUrl = Constants::DEFAULT_SERVICE_2_URL;
        s2.textPrompt = Constants::PROMPT_TEXT;
        s2.formulaPrompt = Constants::PROMPT_FORMULA;
        s2.tablePrompt = Constants::PROMPT_TABLE;
        defaults.append(s2);

        setServiceProfiles(defaults);
        m_settings.setValue("services/switch_mode", Constants::DEFAULT_SERVICE_SWITCH_MODE);
        m_settings.setValue("services/current_id", s1.id);
        m_settings.setValue("services/default_local_id", s1.id);

        m_settings.sync();
    }
    else {
        // 2. 如果是旧版本升级，补全缺失的字段
        bool needsSync = false; // 变量在此处声明

        if (!m_settings.contains("services/switch_mode")) {
            m_settings.setValue("services/switch_mode", Constants::DEFAULT_SERVICE_SWITCH_MODE);
            needsSync = true;
        }
        if (!m_settings.contains("display/renderer_font_size")) {
            m_settings.setValue("display/renderer_font_size", Constants::DEFAULT_RENDERER_FONT_SIZE);
            needsSync = true;
        }
        if (!m_settings.contains("service/auto_start")) {
            m_settings.setValue("service/auto_start", Constants::DEFAULT_AUTO_START_SERVICE);
            needsSync = true;
        }

        // 【新增】补全新增的全局 API Key 和 Model Name
        if (!m_settings.contains("server/api_key")) {
            m_settings.setValue("server/api_key", Constants::DEFAULT_GLOBAL_API_KEY);
            needsSync = true;
        }
        if (!m_settings.contains("server/model_name")) {
            m_settings.setValue("server/model_name", Constants::DEFAULT_GLOBAL_MODEL_NAME);
            needsSync = true;
        }

        if (!m_settings.contains("history/save_enabled")) {
            m_settings.setValue("history/save_enabled", Constants::DEFAULT_SAVE_HISTORY);
        }
        if (!m_settings.contains("history/limit")) {
            m_settings.setValue("history/limit", Constants::DEFAULT_HISTORY_LIMIT);
        }

        if (!m_settings.contains("services/default_local_id")) {
            QList<ServiceProfile> profiles = serviceProfiles();
            for(const auto& p : profiles) {
                if (!p.startCommand.isEmpty()) {
                    m_settings.setValue("services/default_local_id", p.id);
                    break;
                }
            }
            needsSync = true;
        }

        if (!m_settings.contains("network/request_timeout")) {
            m_settings.setValue("network/request_timeout", Constants::DEFAULT_REQUEST_TIMEOUT);
            needsSync = true;
        }

        if (!m_settings.contains("behavior/silent_mode_enabled")) {
            m_settings.setValue("behavior/silent_mode_enabled", Constants::DEFAULT_SILENT_MODE_ENABLED);
            needsSync = true;
        }
        if (!m_settings.contains("behavior/silent_mode_notification_type")) {
            m_settings.setValue("behavior/silent_mode_notification_type", Constants::DEFAULT_SILENT_MODE_NOTIFICATION_TYPE);
            needsSync = true;
        }

        if (!m_settings.contains("floating_ball/size")) {
            m_settings.setValue("floating_ball/size", Constants::DEFAULT_FLOATING_BALL_SIZE);
            needsSync = true;
        }
        if (!m_settings.contains("floating_ball/pos_x")) {
            m_settings.setValue("floating_ball/pos_x", Constants::DEFAULT_FLOATING_BALL_POS_X);
            needsSync = true;
        }
        if (!m_settings.contains("floating_ball/pos_y")) {
            m_settings.setValue("floating_ball/pos_y", Constants::DEFAULT_FLOATING_BALL_POS_Y);
            needsSync = true;
        }
        if (!m_settings.contains("floating_ball/auto_hide_time")) {
            m_settings.setValue("floating_ball/auto_hide_time", Constants::DEFAULT_FLOATING_BALL_AUTO_HIDE_TIME);
            needsSync = true;
        }
        if (!m_settings.contains("floating_ball/always_visible")) {
            m_settings.setValue("floating_ball/always_visible", Constants::DEFAULT_FLOATING_BALL_ALWAYS_VISIBLE);
            needsSync = true;
        }
        if (!m_settings.contains("shortcuts/abort")) {
            m_settings.setValue("shortcuts/abort", Constants::SHORTCUT_ABORT);
            needsSync = true;
        }

        // 强制检查服务列表数据完整性
        QList<ServiceProfile> profiles = serviceProfiles();
        bool listChanged = false;
        for (auto& p : profiles) {
            if (p.serverUrl.isEmpty() && p.name != "远程 API") {
                p.serverUrl = Constants::DEFAULT_SERVER_URL;
                listChanged = true;
            }
            if (p.textPrompt.isEmpty()) { p.textPrompt = Constants::PROMPT_TEXT; listChanged = true; }
            if (p.formulaPrompt.isEmpty()) { p.formulaPrompt = Constants::PROMPT_FORMULA; listChanged = true; }
            if (p.tablePrompt.isEmpty()) { p.tablePrompt = Constants::PROMPT_TABLE; listChanged = true; }
        }

        if (listChanged) {
            setServiceProfiles(profiles);
        } else if (needsSync) {
            m_settings.sync();
        }
    }
}

inline void SettingsManager::sync()
{
    m_settings.sync();
}

// --- 服务配置序列化与反序列化 ---
inline QList<ServiceProfile> SettingsManager::serviceProfiles() const
{
    QList<ServiceProfile> list;
    QString jsonStr = m_settings.value("services/list").toString();
    if (jsonStr.isEmpty()) return list;

    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    if (!doc.isArray()) return list;

    QJsonArray arr = doc.array();
    for (const QJsonValue& val : arr) {
        QJsonObject obj = val.toObject();
        ServiceProfile p;
        p.id = obj["id"].toString();
        p.name = obj["name"].toString();
        p.startCommand = obj["start_command"].toString();
        p.serverUrl = obj["server_url"].toString(Constants::DEFAULT_SERVER_URL);
        p.textPrompt = obj["text_prompt"].toString(Constants::PROMPT_TEXT);
        p.formulaPrompt = obj["formula_prompt"].toString(Constants::PROMPT_FORMULA);
        p.tablePrompt = obj["table_prompt"].toString(Constants::PROMPT_TABLE);

        // 【新增】
        p.apiKey = obj["api_key"].toString();
        p.modelName = obj["model_name"].toString();

        list.append(p);
    }
    return list;
}

inline void SettingsManager::setServiceProfiles(const QList<ServiceProfile>& profiles)
{
    QJsonArray arr;
    for (const ServiceProfile& p : profiles) {
        QJsonObject obj;
        obj["id"] = p.id;
        obj["name"] = p.name;
        obj["start_command"] = p.startCommand;
        obj["server_url"] = p.serverUrl;
        obj["text_prompt"] = p.textPrompt;
        obj["formula_prompt"] = p.formulaPrompt;
        obj["table_prompt"] = p.tablePrompt;

        // 【新增】
        obj["api_key"] = p.apiKey;
        obj["model_name"] = p.modelName;

        arr.append(obj);
    }
    QJsonDocument doc(arr);
    m_settings.setValue("services/list", QString::fromUtf8(doc.toJson()));
    emit serviceProfilesChanged();
}

inline SettingsManager::ServiceSwitchMode SettingsManager::serviceSwitchMode() const
{
    return static_cast<ServiceSwitchMode>(m_settings.value("services/switch_mode", Constants::DEFAULT_SERVICE_SWITCH_MODE).toInt());
}

inline void SettingsManager::setServiceSwitchMode(ServiceSwitchMode mode)
{
    m_settings.setValue("services/switch_mode", static_cast<int>(mode));
}

inline QString SettingsManager::currentServiceId() const
{
    return m_settings.value("services/current_id").toString();
}

inline void SettingsManager::setCurrentServiceId(const QString& id)
{
    if (currentServiceId() != id) {
        m_settings.setValue("services/current_id", id);
        emit currentServiceIdChanged(id);
    }
}

inline QString SettingsManager::defaultLocalServiceId() const
{
    return m_settings.value("services/default_local_id").toString();
}

inline void SettingsManager::setDefaultLocalServiceId(const QString& id)
{
    if (defaultLocalServiceId() != id) {
        m_settings.setValue("services/default_local_id", id);
        emit defaultLocalServiceIdChanged(id);
    }
}

inline ServiceProfile SettingsManager::getServiceProfile(const QString& id) const
{
    auto profiles = serviceProfiles();
    for (const auto& p : profiles) {
        if (p.id == id) return p;
    }
    return ServiceProfile();
}

// --- 其他设置实现保持不变 ---

inline QString SettingsManager::serverUrl() const {
    return m_settings.value("server/url", Constants::DEFAULT_SERVER_URL).toString();
}
inline void SettingsManager::setServerUrl(const QString& url) {
    if (serverUrl() != url) { m_settings.setValue("server/url", url); emit serverUrlChanged(url); }
}

inline QString SettingsManager::mathFont() const {
    return m_settings.value("display/math_font", Constants::DEFAULT_MATH_FONT).toString();
}
inline void SettingsManager::setMathFont(const QString& font) {
    if (mathFont() != font) { m_settings.setValue("display/math_font", font); emit mathFontChanged(font); }
}

inline QString SettingsManager::screenshotShortcut() const { return m_settings.value("shortcuts/screenshot", Constants::SHORTCUT_SCREENSHOT).toString(); }
inline QString SettingsManager::textRecognizeShortcut() const { return m_settings.value("shortcuts/text_recognize", Constants::SHORTCUT_TEXT).toString(); }
inline QString SettingsManager::formulaRecognizeShortcut() const { return m_settings.value("shortcuts/formula_recognize", Constants::SHORTCUT_FORMULA).toString(); }
inline QString SettingsManager::tableRecognizeShortcut() const { return m_settings.value("shortcuts/table_recognize", Constants::SHORTCUT_TABLE).toString(); }
inline QString SettingsManager::externalProcessShortcut() const { return m_settings.value("shortcuts/external_process", Constants::SHORTCUT_EXTERNAL_PROCESS).toString(); }

inline void SettingsManager::setScreenshotShortcut(const QString& key) { if (screenshotShortcut() != key) { m_settings.setValue("shortcuts/screenshot", key); emit shortcutsChanged(); } }
inline void SettingsManager::setTextRecognizeShortcut(const QString& key) { if (textRecognizeShortcut() != key) { m_settings.setValue("shortcuts/text_recognize", key); emit shortcutsChanged(); } }
inline void SettingsManager::setFormulaRecognizeShortcut(const QString& key) { if (formulaRecognizeShortcut() != key) { m_settings.setValue("shortcuts/formula_recognize", key); emit shortcutsChanged(); } }
inline void SettingsManager::setTableRecognizeShortcut(const QString& key) { if (tableRecognizeShortcut() != key) { m_settings.setValue("shortcuts/table_recognize", key); emit shortcutsChanged(); } }
inline void SettingsManager::setExternalProcessShortcut(const QString& key) { if (externalProcessShortcut() != key) { m_settings.setValue("shortcuts/external_process", key); emit externalProcessShortcutChanged(key); } }

inline bool SettingsManager::autoUseLastPrompt() const { return m_settings.value("behavior/auto_use_last_prompt", true).toBool(); }
inline void SettingsManager::setAutoUseLastPrompt(bool enabled) { if (autoUseLastPrompt() != enabled) { m_settings.setValue("behavior/auto_use_last_prompt", enabled); emit autoUseLastPromptChanged(enabled); } }

inline QString SettingsManager::displayMathEnvironment() const { return m_settings.value("display_math_env", Constants::DEFAULT_DISPLAY_MATH_ENV).toString(); }
inline void SettingsManager::setDisplayMathEnvironment(const QString& env) { if (displayMathEnvironment() != env) { m_settings.setValue("display_math_env", env); emit displayMathEnvironmentChanged(env); } }

inline QString SettingsManager::externalProcessorCommand() const { return m_settings.value("external_processor/command", Constants::DEFAULT_EXTERNAL_PROCESSOR).toString(); }
inline void SettingsManager::setExternalProcessorCommand(const QString& cmd) { if (externalProcessorCommand() != cmd) { m_settings.setValue("external_processor/command", cmd); emit externalProcessorCommandChanged(cmd); } }

inline bool SettingsManager::autoCopyResult() const { return m_settings.value("behavior/auto_copy_result", Constants::DEFAULT_AUTO_COPY_RESULT).toBool(); }
inline void SettingsManager::setAutoCopyResult(bool enabled) { if (autoCopyResult() != enabled) { m_settings.setValue("behavior/auto_copy_result", enabled); emit autoCopyResultChanged(enabled); } }

inline bool SettingsManager::autoRecognizeOnScreenshot() const { return m_settings.value("behavior/auto_recognize_screenshot", Constants::DEFAULT_AUTO_RECOGNIZE_SCREENSHOT).toBool(); }
inline void SettingsManager::setAutoRecognizeOnScreenshot(bool enabled) { if (autoRecognizeOnScreenshot() != enabled) { m_settings.setValue("behavior/auto_recognize_screenshot", enabled); emit autoRecognizeOnScreenshotChanged(enabled); } }

inline bool SettingsManager::autoExternalProcessBeforeCopy() const { return m_settings.value("behavior/auto_external_process_before_copy", Constants::DEFAULT_AUTO_EXTERNAL_PROCESS_BEFORE_COPY).toBool(); }
inline void SettingsManager::setAutoExternalProcessBeforeCopy(bool enabled) { if (autoExternalProcessBeforeCopy() != enabled) { m_settings.setValue("behavior/auto_external_process_before_copy", enabled); emit autoExternalProcessBeforeCopyChanged(enabled); } }

inline bool SettingsManager::autoStartService() const { return m_settings.value("service/auto_start", Constants::DEFAULT_AUTO_START_SERVICE).toBool(); }
inline void SettingsManager::setAutoStartService(bool enabled) { if (autoStartService() != enabled) { m_settings.setValue("service/auto_start", enabled); emit autoStartServiceChanged(enabled); } }

inline QString SettingsManager::serviceStartCommand() const { return m_settings.value("service/start_command", Constants::DEFAULT_SERVICE_START_COMMAND).toString(); }
inline void SettingsManager::setServiceStartCommand(const QString& cmd) { if (serviceStartCommand() != cmd) { m_settings.setValue("service/start_command", cmd); emit serviceStartCommandChanged(cmd); } }

inline int SettingsManager::serviceIdleTimeout() const { return m_settings.value("service/idle_timeout", Constants::DEFAULT_SERVICE_IDLE_TIMEOUT).toInt(); }
inline void SettingsManager::setServiceIdleTimeout(int minutes) { if (serviceIdleTimeout() != minutes) { m_settings.setValue("service/idle_timeout", minutes); emit serviceIdleTimeoutChanged(minutes); } }

inline QString SettingsManager::requestParameters() const { return m_settings.value("network/request_parameters", Constants::DEFAULT_REQUEST_PARAMETERS).toString(); }
inline void SettingsManager::setRequestParameters(const QString& json) { if (requestParameters() != json) { m_settings.setValue("network/request_parameters", json); emit requestParametersChanged(json); } }

inline int SettingsManager::imageViewMode() const { return m_settings.value("display/view_mode", Constants::DEFAULT_IMAGE_VIEW_MODE).toInt(); }
inline void SettingsManager::setImageViewMode(int mode) { if (imageViewMode() != mode) { m_settings.setValue("display/view_mode", mode); emit imageViewModeChanged(mode); } }

inline QString SettingsManager::textPrompt() const { return m_settings.value("prompts/text", Constants::PROMPT_TEXT).toString(); }
inline void SettingsManager::setTextPrompt(const QString& prompt) { if (textPrompt() != prompt) { m_settings.setValue("prompts/text", prompt); emit textPromptChanged(prompt); } }
inline QString SettingsManager::formulaPrompt() const { return m_settings.value("prompts/formula", Constants::PROMPT_FORMULA).toString(); }
inline void SettingsManager::setFormulaPrompt(const QString& prompt) { if (formulaPrompt() != prompt) { m_settings.setValue("prompts/formula", prompt); emit formulaPromptChanged(prompt); } }
inline QString SettingsManager::tablePrompt() const { return m_settings.value("prompts/table", Constants::PROMPT_TABLE).toString(); }
inline void SettingsManager::setTablePrompt(const QString& prompt) { if (tablePrompt() != prompt) { m_settings.setValue("prompts/table", prompt); emit tablePromptChanged(prompt); } }

inline int SettingsManager::rendererFontSize() const { return m_settings.value("display/renderer_font_size", Constants::DEFAULT_RENDERER_FONT_SIZE).toInt(); }
inline void SettingsManager::setRendererFontSize(int size) { if (rendererFontSize() != size) { m_settings.setValue("display/renderer_font_size", size); emit rendererFontSizeChanged(size); } }

inline int SettingsManager::sourceEditorFontSize() const { return m_settings.value("display/source_editor_font_size", Constants::DEFAULT_SOURCE_EDITOR_FONT_SIZE).toInt(); }
inline void SettingsManager::setSourceEditorFontSize(int size) { if (sourceEditorFontSize() != size) { m_settings.setValue("display/source_editor_font_size", size); emit sourceEditorFontSizeChanged(size); } }



// --- 新增实现 ---

inline QString SettingsManager::textProcessorCommand() const {
    return m_settings.value("processor/text_cmd", Constants::DEFAULT_TEXT_PROCESSOR_CMD).toString();
}
inline void SettingsManager::setTextProcessorCommand(const QString& cmd) {
    if (textProcessorCommand() != cmd) {
        m_settings.setValue("processor/text_cmd", cmd);
        emit textProcessorCommandChanged(cmd);
    }
}

inline QString SettingsManager::formulaProcessorCommand() const {
    return m_settings.value("processor/formula_cmd", Constants::DEFAULT_FORMULA_PROCESSOR_CMD).toString();
}
inline void SettingsManager::setFormulaProcessorCommand(const QString& cmd) {
    if (formulaProcessorCommand() != cmd) {
        m_settings.setValue("processor/formula_cmd", cmd);
        emit formulaProcessorCommandChanged(cmd);
    }
}

inline QString SettingsManager::tableProcessorCommand() const {
    return m_settings.value("processor/table_cmd", Constants::DEFAULT_TABLE_PROCESSOR_CMD).toString();
}
inline void SettingsManager::setTableProcessorCommand(const QString& cmd) {
    if (tableProcessorCommand() != cmd) {
        m_settings.setValue("processor/table_cmd", cmd);
        emit tableProcessorCommandChanged(cmd);
    }
}

inline QString SettingsManager::pureMathProcessorCommand() const {
    return m_settings.value("processor/pure_math_cmd", Constants::DEFAULT_PURE_MATH_PROCESSOR_CMD).toString();
}
inline void SettingsManager::setPureMathProcessorCommand(const QString& cmd) {
    if (pureMathProcessorCommand() != cmd) {
        m_settings.setValue("processor/pure_math_cmd", cmd);
        emit pureMathProcessorCommandChanged(cmd);
    }
}

inline QString SettingsManager::textProcessorShortcut() const {
    return m_settings.value("shortcuts/text_processor", Constants::SHORTCUT_TEXT_PROCESSOR).toString();
}
inline void SettingsManager::setTextProcessorShortcut(const QString& key) {
    if (textProcessorShortcut() != key) {
        m_settings.setValue("shortcuts/text_processor", key);
        emit shortcutsChanged();
    }
}

inline QString SettingsManager::formulaProcessorShortcut() const {
    return m_settings.value("shortcuts/formula_processor", Constants::SHORTCUT_FORMULA_PROCESSOR).toString();
}
inline void SettingsManager::setFormulaProcessorShortcut(const QString& key) {
    if (formulaProcessorShortcut() != key) {
        m_settings.setValue("shortcuts/formula_processor", key);
        emit shortcutsChanged();
    }
}

inline QString SettingsManager::tableProcessorShortcut() const {
    return m_settings.value("shortcuts/table_processor", Constants::SHORTCUT_TABLE_PROCESSOR).toString();
}
inline void SettingsManager::setTableProcessorShortcut(const QString& key) {
    if (tableProcessorShortcut() != key) {
        m_settings.setValue("shortcuts/table_processor", key);
        emit shortcutsChanged();
    }
}

inline QString SettingsManager::pureMathProcessorShortcut() const {
    return m_settings.value("shortcuts/pure_math_processor", Constants::SHORTCUT_PURE_MATH_PROCESSOR).toString();
}
inline void SettingsManager::setPureMathProcessorShortcut(const QString& key) {
    if (pureMathProcessorShortcut() != key) {
        m_settings.setValue("shortcuts/pure_math_processor", key);
        emit shortcutsChanged();
    }
}

inline bool SettingsManager::textProcessorEnabled() const {
    return m_settings.value("processor/text_enabled", Constants::DEFAULT_PROCESSOR_ENABLED).toBool();
}
inline void SettingsManager::setTextProcessorEnabled(bool enabled) {
    if (textProcessorEnabled() != enabled) {
        m_settings.setValue("processor/text_enabled", enabled);
        emit textProcessorEnabledChanged(enabled);
    }
}

inline bool SettingsManager::formulaProcessorEnabled() const {
    return m_settings.value("processor/formula_enabled", Constants::DEFAULT_PROCESSOR_ENABLED).toBool();
}
inline void SettingsManager::setFormulaProcessorEnabled(bool enabled) {
    if (formulaProcessorEnabled() != enabled) {
        m_settings.setValue("processor/formula_enabled", enabled);
        emit formulaProcessorEnabledChanged(enabled);
    }
}

inline bool SettingsManager::tableProcessorEnabled() const {
    return m_settings.value("processor/table_enabled", Constants::DEFAULT_PROCESSOR_ENABLED).toBool();
}
inline void SettingsManager::setTableProcessorEnabled(bool enabled) {
    if (tableProcessorEnabled() != enabled) {
        m_settings.setValue("processor/table_enabled", enabled);
        emit tableProcessorEnabledChanged(enabled);
    }
}

inline bool SettingsManager::pureMathProcessorEnabled() const {
    return m_settings.value("processor/pure_math_enabled", Constants::DEFAULT_PROCESSOR_ENABLED).toBool();
}
inline void SettingsManager::setPureMathProcessorEnabled(bool enabled) {
    if (pureMathProcessorEnabled() != enabled) {
        m_settings.setValue("processor/pure_math_enabled", enabled);
        emit pureMathProcessorEnabledChanged(enabled);
    }
}

inline QString SettingsManager::globalApiKey() const {
    return m_settings.value("server/api_key", Constants::DEFAULT_GLOBAL_API_KEY).toString();
}

inline void SettingsManager::setGlobalApiKey(const QString& key) {
    if (globalApiKey() != key) {
        m_settings.setValue("server/api_key", key);
        emit globalApiKeyChanged(key);
    }
}

inline QString SettingsManager::globalModelName() const {
    return m_settings.value("server/model_name", Constants::DEFAULT_GLOBAL_MODEL_NAME).toString();
}

inline void SettingsManager::setGlobalModelName(const QString& name) {
    if (globalModelName() != name) {
        m_settings.setValue("server/model_name", name);
        emit globalModelNameChanged(name);
    }
}


inline int SettingsManager::requestTimeout() const {
    // 默认值取 Constants::DEFAULT_REQUEST_TIMEOUT
    return m_settings.value("network/request_timeout", Constants::DEFAULT_REQUEST_TIMEOUT).toInt();
}

inline void SettingsManager::setRequestTimeout(int seconds) {
    if (requestTimeout() != seconds) {
        m_settings.setValue("network/request_timeout", seconds);
        emit requestTimeoutChanged(seconds);
    }
}

inline bool SettingsManager::saveHistoryEnabled() const {
    return m_settings.value("history/save_enabled", Constants::DEFAULT_SAVE_HISTORY).toBool();
}

inline void SettingsManager::setSaveHistoryEnabled(bool enabled) {
    if (saveHistoryEnabled() != enabled) {
        m_settings.setValue("history/save_enabled", enabled);
        emit saveHistoryEnabledChanged(enabled);
    }
}

inline int SettingsManager::historyLimit() const {
    return m_settings.value("history/limit", Constants::DEFAULT_HISTORY_LIMIT).toInt();
}

inline void SettingsManager::setHistoryLimit(int limit) {
    if (historyLimit() != limit) {
        m_settings.setValue("history/limit", limit);
        emit historyLimitChanged(limit);
    }
}


inline bool SettingsManager::silentModeEnabled() const {
    return m_settings.value("behavior/silent_mode_enabled", Constants::DEFAULT_SILENT_MODE_ENABLED).toBool();
}

inline void SettingsManager::setSilentModeEnabled(bool enabled) {
    if (silentModeEnabled() != enabled) {
        m_settings.setValue("behavior/silent_mode_enabled", enabled);
        emit silentModeEnabledChanged(enabled);
    }
}

inline QString SettingsManager::silentModeNotificationType() const {
    return m_settings.value("behavior/silent_mode_notification_type", Constants::DEFAULT_SILENT_MODE_NOTIFICATION_TYPE).toString();
}

inline void SettingsManager::setSilentModeNotificationType(const QString& type) {
    if (silentModeNotificationType() != type) {
        m_settings.setValue("behavior/silent_mode_notification_type", type);
        emit silentModeNotificationTypeChanged(type);
    }
}


inline int SettingsManager::floatingBallSize() const {
    return m_settings.value("floating_ball/size", Constants::DEFAULT_FLOATING_BALL_SIZE).toInt();
}
inline void SettingsManager::setFloatingBallSize(int size) {
    if (floatingBallSize() != size) {
        m_settings.setValue("floating_ball/size", size);
        emit floatingBallSizeChanged(size);
    }
}

inline int SettingsManager::floatingBallPosX() const {
    return m_settings.value("floating_ball/pos_x", Constants::DEFAULT_FLOATING_BALL_POS_X).toInt();
}
inline void SettingsManager::setFloatingBallPosX(int x) {
    m_settings.setValue("floating_ball/pos_x", x);
}

inline int SettingsManager::floatingBallPosY() const {
    return m_settings.value("floating_ball/pos_y", Constants::DEFAULT_FLOATING_BALL_POS_Y).toInt();
}
inline void SettingsManager::setFloatingBallPosY(int y) {
    m_settings.setValue("floating_ball/pos_y", y);
}

inline int SettingsManager::floatingBallAutoHideTime() const {
    return m_settings.value("floating_ball/auto_hide_time", Constants::DEFAULT_FLOATING_BALL_AUTO_HIDE_TIME).toInt();
}
inline void SettingsManager::setFloatingBallAutoHideTime(int time) {
    if (floatingBallAutoHideTime() != time) {
        m_settings.setValue("floating_ball/auto_hide_time", time);
        emit floatingBallAutoHideTimeChanged(time);
    }
}

inline bool SettingsManager::floatingBallAlwaysVisible() const {
    return m_settings.value("floating_ball/always_visible", Constants::DEFAULT_FLOATING_BALL_ALWAYS_VISIBLE).toBool();
}
inline void SettingsManager::setFloatingBallAlwaysVisible(bool visible) {
    if (floatingBallAlwaysVisible() != visible) {
        m_settings.setValue("floating_ball/always_visible", visible);
        emit floatingBallAlwaysVisibleChanged(visible);
    }
}

inline QString SettingsManager::abortShortcut() const {
    return m_settings.value("shortcuts/abort", Constants::SHORTCUT_ABORT).toString();
}

inline void SettingsManager::setAbortShortcut(const QString& key) {
    if (abortShortcut() != key) {
        m_settings.setValue("shortcuts/abort", key);
        emit shortcutsChanged();
    }
}

#endif // SETTINGSMANAGER_IMPL_H
