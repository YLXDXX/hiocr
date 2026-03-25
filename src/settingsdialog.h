#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QComboBox>

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
    QLineEdit* m_scScreenshotEdit;
    QLineEdit* m_scTextEdit;
    QLineEdit* m_scFormulaEdit;
    QLineEdit* m_scTableEdit;

    // 行为设置
    QCheckBox* m_autoUseLastPromptCheck;

    // 显示设置
    QComboBox* m_displayMathCombo;
    QComboBox* m_mathFontCombo; // 【新增】字体选择框

    QPushButton* m_saveBtn;
    QPushButton* m_defaultBtn;

    QCheckBox* m_autoCopyCheck;
    QCheckBox* m_autoRecognizeCheck;
};

#endif // SETTINGSDIALOG_H
