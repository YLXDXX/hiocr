#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include "shortcutedit.h"
#include "settingsmanager.h"

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QPlainTextEdit>
#include <QListWidget>

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

private slots:
    void onSaveClicked();
    void onRestoreDefaults();

    void onAddService();
    void onRemoveService();
    void onServiceDetailChanged();

private:
    void loadSettings();
    void setupUi();
    void populateServiceList();
    void saveCurrentServiceToTemp(int row);

    QLineEdit* m_serverUrlEdit;
    QLineEdit* m_globalApiKeyEdit;
    QLineEdit* m_globalModelNameEdit;

    ShortcutEdit* m_scScreenshotEdit;
    ShortcutEdit* m_scTextEdit;
    ShortcutEdit* m_scFormulaEdit;
    ShortcutEdit* m_scTableEdit;
    ShortcutEdit* m_abortScEdit;
    ShortcutEdit* m_scTableEd;

    // --- 新增：脚本配置控件 ---
    QLineEdit* m_textProcessorEdit;
    ShortcutEdit* m_textProcessorScEdit;

    QLineEdit* m_formulaProcessorEdit;
    ShortcutEdit* m_formulaProcessorScEdit;

    QLineEdit* m_tableProcessorEdit;
    ShortcutEdit* m_tableProcessorScEdit;

    QLineEdit* m_pureMathProcessorEdit;
    ShortcutEdit* m_pureMathProcessorScEdit;

    QCheckBox* m_autoUseLastPromptCheck;

    QComboBox* m_displayMathCombo;
    QComboBox* m_mathFontCombo;

    QPushButton* m_saveBtn;
    QPushButton* m_defaultBtn;

    QCheckBox* m_autoCopyCheck;
    QCheckBox* m_autoRecognizeCheck;

    // 【修改】服务管理控件
    QComboBox* m_serviceSwitchModeCombo;
    QListWidget* m_serviceListWidget;
    QLineEdit* m_serviceNameEdit;
    QLineEdit* m_serviceCommandEdit;
    QLineEdit* m_serviceUrlEdit;
    QLineEdit* m_serviceApiKeyEdit;
    QLineEdit* m_serviceModelEdit;
    QLineEdit* m_serviceTextPromptEdit;
    QLineEdit* m_serviceFormulaPromptEdit;
    QLineEdit* m_serviceTablePromptEdit;
    QPushButton* m_addServiceBtn;
    QPushButton* m_removeServiceBtn;

    QSpinBox* m_timeoutSpin; // 【新增】
    QPlainTextEdit* m_requestParamsEdit;

    // 全局默认提示词
    QLineEdit* m_globalTextPromptEdit;
    QLineEdit* m_globalFormulaPromptEdit;
    QLineEdit* m_globalTablePromptEdit;

    QCheckBox* m_autoExternalProcessCheck;

    QSpinBox* m_rendererFontSpin;
    QSpinBox* m_sourceEditorFontSpin;
    QSpinBox* m_serviceIdleTimeoutSpin;

    // 【新增】自动启动服务开关
    QCheckBox* m_autoStartServiceCheck;

    // 临时数据
    QList<ServiceProfile> m_tempServiceProfiles;
    int m_currentEditingServiceIndex = -1;

    QCheckBox* m_saveHistoryCheck;
    QSpinBox* m_historyLimitSpin;

    QCheckBox* m_silentModeCheck;
    QComboBox* m_silentModeNotificationCombo;

    QSpinBox* m_floatingBallSizeSpin;
    QSpinBox* m_floatingBallAutoHideTimeSpin;
    QCheckBox* m_floatingBallAlwaysVisibleCheck;
};

#include "settingsdialog_impl.h"

#endif // SETTINGSDIALOG_H
