#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H
#include "shortcutedit.h"

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QPlainTextEdit> // 引入多行编辑框


class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

private slots:
    void onSaveClicked();
    void onRestoreDefaults();

private:
    void loadSettings();
    void setupUi();

    QLineEdit* m_serverUrlEdit;
    QLineEdit* m_externalProcessorEdit;

    // 快捷键编辑
    ShortcutEdit* m_scScreenshotEdit;
    ShortcutEdit* m_scTextEdit;
    ShortcutEdit* m_scFormulaEdit;
    ShortcutEdit* m_scTableEdit;
    ShortcutEdit* m_scExternalProcessEdit; // 【新增】

    // 行为设置
    QCheckBox* m_autoUseLastPromptCheck;

    // 显示设置
    QComboBox* m_displayMathCombo;
    QComboBox* m_mathFontCombo; // 【新增】字体选择框

    QPushButton* m_saveBtn;
    QPushButton* m_defaultBtn;

    QCheckBox* m_autoCopyCheck;
    QCheckBox* m_autoRecognizeCheck;

    QCheckBox* m_autoStartServiceCheck;
    QLineEdit* m_serviceStartCommandEdit;
    QSpinBox* m_serviceIdleTimeoutSpin; // 需要包含 QSpinBox


    // 【新增】请求参数编辑框
    QPlainTextEdit* m_requestParamsEdit;


    // 提示词编辑
    QLineEdit* m_textPromptEdit;
    QLineEdit* m_formulaPromptEdit;
    QLineEdit* m_tablePromptEdit;

    QCheckBox* m_autoExternalProcessCheck;

    // 【新增】字体大小控件
    QSpinBox* m_rendererFontSpin;
    QSpinBox* m_sourceEditorFontSpin;

};

#endif // SETTINGSDIALOG_H
