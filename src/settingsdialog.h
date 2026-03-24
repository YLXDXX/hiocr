#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>

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

    // 快捷键编辑
    QLineEdit* m_scScreenshotEdit;
    QLineEdit* m_scTextEdit;
    QLineEdit* m_scFormulaEdit;
    QLineEdit* m_scTableEdit;

    QCheckBox* m_autoUseLastPromptCheck;
    QPushButton* m_saveBtn;
    QPushButton* m_defaultBtn;
};

#endif // SETTINGSDIALOG_H
