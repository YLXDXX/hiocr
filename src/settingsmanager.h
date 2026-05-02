#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include "contenttype.h"
#include <QObject>
#include <QSettings>
#include <QString>
#include <QList>

#include "constants.h"
#include <QFile>
#include <QStandardPaths>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QUuid>


// 【新增】服务配置结构体
struct ServiceProfile {
    QString id;
    QString name;
    QString startCommand;
    QString serverUrl;
    QString textPrompt;
    QString formulaPrompt;
    QString tablePrompt;

    // 【新增】
    QString apiKey;      // API 密钥
    QString modelName;   // 模型名称 (如 qwen-plus, deepseek-chat)

    bool isEmpty() const { return id.isEmpty() && name.isEmpty(); }
};

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

    QString globalApiKey() const;
    void setGlobalApiKey(const QString& key);

    QString globalModelName() const;
    void setGlobalModelName(const QString& name);

    // 快捷键设置
    QString screenshotShortcut() const;
    QString textRecognizeShortcut() const;
    QString formulaRecognizeShortcut() const;
    QString tableRecognizeShortcut() const;

    void setScreenshotShortcut(const QString& key);
    void setTextRecognizeShortcut(const QString& key);
    void setFormulaRecognizeShortcut(const QString& key);
    void setTableRecognizeShortcut(const QString& key);

    QString abortShortcut() const;
    void setAbortShortcut(const QString& key);

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


    // 【新增】复制前自动调用外部程序设置
    bool autoExternalProcessBeforeCopy() const;
    void setAutoExternalProcessBeforeCopy(bool enabled);

    QString externalProcessShortcut() const;
    void setExternalProcessShortcut(const QString& key);

    // 【新增】字体大小设置
    int rendererFontSize() const;
    void setRendererFontSize(int size);

    int sourceEditorFontSize() const;
    void setSourceEditorFontSize(int size);


    // 【新增】服务管理接口
    enum ServiceSwitchMode {
        SingleService = 0,  // 仅保留一个
        ParallelServices = 1 // 保留全部
    };

    QList<ServiceProfile> serviceProfiles() const;
    void setServiceProfiles(const QList<ServiceProfile>& profiles);

    ServiceSwitchMode serviceSwitchMode() const;
    void setServiceSwitchMode(ServiceSwitchMode mode);

    // 当前激活的服务 ID（空字符串表示使用全局设置）
    QString currentServiceId() const;
    void setCurrentServiceId(const QString& id);

    // 【新增】默认本地服务 ID（用于自动启动时切换）
    QString defaultLocalServiceId() const;
    void setDefaultLocalServiceId(const QString& id);

    // 获取指定 ID 的服务配置
    ServiceProfile getServiceProfile(const QString& id) const;


    // --- 分类型外部处理脚本配置 ---
    QString textProcessorCommand() const;
    void setTextProcessorCommand(const QString& cmd);

    QString formulaProcessorCommand() const;
    void setFormulaProcessorCommand(const QString& cmd);

    QString tableProcessorCommand() const;
    void setTableProcessorCommand(const QString& cmd);

    QString pureMathProcessorCommand() const;
    void setPureMathProcessorCommand(const QString& cmd);

    // --- 分类型外部处理脚本快捷键 ---
    QString textProcessorShortcut() const;
    void setTextProcessorShortcut(const QString& key);

    QString formulaProcessorShortcut() const;
    void setFormulaProcessorShortcut(const QString& key);

    QString tableProcessorShortcut() const;
    void setTableProcessorShortcut(const QString& key);

    QString pureMathProcessorShortcut() const;
    void setPureMathProcessorShortcut(const QString& key);

    // --- 分类型脚本启用开关 (菜单栏 Checkbox 状态) ---
    bool textProcessorEnabled() const;
    void setTextProcessorEnabled(bool enabled);

    bool formulaProcessorEnabled() const;
    void setFormulaProcessorEnabled(bool enabled);

    bool tableProcessorEnabled() const;
    void setTableProcessorEnabled(bool enabled);

    bool pureMathProcessorEnabled() const;
    void setPureMathProcessorEnabled(bool enabled);

    // 【新增】请求超时设置
    int requestTimeout() const;
    void setRequestTimeout(int seconds);

    bool saveHistoryEnabled() const;
    void setSaveHistoryEnabled(bool enabled);
    int historyLimit() const;
    void setHistoryLimit(int limit);

    // 静默模式设置
    bool silentModeEnabled() const;
    void setSilentModeEnabled(bool enabled);

    QString silentModeNotificationType() const;
    void setSilentModeNotificationType(const QString& type);

    // 悬浮球设置
    int floatingBallSize() const;
    void setFloatingBallSize(int size);

    int floatingBallPosX() const;
    void setFloatingBallPosX(int x);

    int floatingBallPosY() const;
    void setFloatingBallPosY(int y);

    int floatingBallAutoHideTime() const;
    void setFloatingBallAutoHideTime(int time);

    bool floatingBallAlwaysVisible() const;
    void setFloatingBallAlwaysVisible(bool visible);

    // --- LaTeX代码格式化工具设置 ---
    enum FormatterOrder {
        FormatFirst = 0,
        ProcessFirst = 1
    };

    bool formatterEnabled() const;
    void setFormatterEnabled(bool enabled);

    QString formatterCommand() const;
    void setFormatterCommand(const QString& cmd);

    FormatterOrder formatterOrder() const;
    void setFormatterOrder(FormatterOrder order);

signals:
    void serviceProfilesChanged();
    void currentServiceIdChanged(const QString& id);
    void defaultLocalServiceIdChanged(const QString& id);

    void serverUrlChanged(const QString& url);
    void globalApiKeyChanged(const QString& key);
    void globalModelNameChanged(const QString& name);
    void shortcutsChanged();
    void autoUseLastPromptChanged(bool enabled);
    void displayMathEnvironmentChanged(const QString& env);
    void mathFontChanged(const QString& font);
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

    void autoExternalProcessBeforeCopyChanged(bool enabled);

    void externalProcessShortcutChanged(const QString& key);

    void rendererFontSizeChanged(int size);
    void sourceEditorFontSizeChanged(int size);

    void textProcessorCommandChanged(const QString& cmd);
    void formulaProcessorCommandChanged(const QString& cmd);
    void tableProcessorCommandChanged(const QString& cmd);
    void pureMathProcessorCommandChanged(const QString& cmd);

    void textProcessorEnabledChanged(bool enabled);
    void formulaProcessorEnabledChanged(bool enabled);
    void tableProcessorEnabledChanged(bool enabled);
    void pureMathProcessorEnabledChanged(bool enabled);

    void requestTimeoutChanged(int seconds);
    void saveHistoryEnabledChanged(bool enabled);
    void historyLimitChanged(int limit);

    void silentModeEnabledChanged(bool enabled);
    void silentModeNotificationTypeChanged(const QString& type);

    void floatingBallSizeChanged(int size);
    void floatingBallAutoHideTimeChanged(int time);
    void floatingBallAlwaysVisibleChanged(bool visible);

    void formatterEnabledChanged(bool enabled);
    void formatterCommandChanged(const QString& cmd);
    void formatterOrderChanged(int order);

private:
    explicit SettingsManager(QObject* parent = nullptr);
    void initializeDefaults();
    QSettings m_settings;
};

#include "settingsmanager_impl.h"

#endif // SETTINGSMANAGER_H
