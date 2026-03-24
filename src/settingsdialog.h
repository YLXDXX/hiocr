#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QComboBox> // 新增：引入 QComboBox 头文件

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

    QLineEdit* m_externalProcessorEdit; // 新增

    // 快捷键编辑
    QLineEdit* m_scScreenshotEdit;
    QLineEdit* m_scTextEdit;
    QLineEdit* m_scFormulaEdit;
    QLineEdit* m_scTableEdit;

    // 行为设置
    QCheckBox* m_autoUseLastPromptCheck;

    // 【新增】显示设置：行间公式环境选择
    QComboBox* m_displayMathCombo;

    QPushButton* m_saveBtn;
    QPushButton* m_defaultBtn;

    QCheckBox* m_autoCopyCheck;           // 新增
    QCheckBox* m_autoRecognizeCheck;      // 新增
};

#endif // SETTINGSDIALOG_H
