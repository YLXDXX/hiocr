#include "settingsdialog.h"
#include "settingsmanager.h"
#include "constants.h" // 引入常量
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDialogButtonBox>

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
    setupUi();
    loadSettings();
}

void SettingsDialog::setupUi()
{
    setWindowTitle("设置");
    resize(400, 300);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // --- 服务器设置 ---
    QGroupBox* serverGroup = new QGroupBox("服务器设置");
    QFormLayout* serverLayout = new QFormLayout(serverGroup);
    m_serverUrlEdit = new QLineEdit();
    serverLayout->addRow("服务器地址:", m_serverUrlEdit);
    mainLayout->addWidget(serverGroup);

    // --- 快捷键设置 ---
    QGroupBox* shortcutGroup = new QGroupBox("快捷键设置");
    QFormLayout* shortcutLayout = new QFormLayout(shortcutGroup);
    m_scScreenshotEdit = new QLineEdit();
    m_scTextEdit = new QLineEdit();
    m_scFormulaEdit = new QLineEdit();
    m_scTableEdit = new QLineEdit();
    shortcutLayout->addRow("截图:", m_scScreenshotEdit);
    shortcutLayout->addRow("文字识别:", m_scTextEdit);
    shortcutLayout->addRow("公式识别:", m_scFormulaEdit);
    shortcutLayout->addRow("表格识别:", m_scTableEdit);
    mainLayout->addWidget(shortcutGroup);

    // --- 行为设置 ---
    QGroupBox* behaviorGroup = new QGroupBox("行为设置");
    QVBoxLayout* behaviorLayout = new QVBoxLayout(behaviorGroup);
    m_autoUseLastPromptCheck = new QCheckBox("自动使用上次识别类型");
    behaviorLayout->addWidget(m_autoUseLastPromptCheck);
    mainLayout->addWidget(behaviorGroup);

    // --- 按钮 ---
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::RestoreDefaults);

    // 连接保存按钮
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::onSaveClicked);

    // 【修复】正确连接恢复默认按钮
    QPushButton* restoreBtn = buttonBox->button(QDialogButtonBox::RestoreDefaults);
    if (restoreBtn) {
        connect(restoreBtn, &QPushButton::clicked, this, &SettingsDialog::onRestoreDefaults);
    }

    mainLayout->addWidget(buttonBox);
}

void SettingsDialog::loadSettings()
{
    SettingsManager* s = SettingsManager::instance();
    m_serverUrlEdit->setText(s->serverUrl());
    m_scScreenshotEdit->setText(s->screenshotShortcut());
    m_scTextEdit->setText(s->textRecognizeShortcut());
    m_scFormulaEdit->setText(s->formulaRecognizeShortcut());
    m_scTableEdit->setText(s->tableRecognizeShortcut());
    m_autoUseLastPromptCheck->setChecked(s->autoUseLastPrompt());
}

void SettingsDialog::onSaveClicked()
{
    SettingsManager* s = SettingsManager::instance();
    s->setServerUrl(m_serverUrlEdit->text());
    s->setScreenshotShortcut(m_scScreenshotEdit->text());
    s->setTextRecognizeShortcut(m_scTextEdit->text());
    s->setFormulaRecognizeShortcut(m_scFormulaEdit->text());
    s->setTableRecognizeShortcut(m_scTableEdit->text());
    s->setAutoUseLastPrompt(m_autoUseLastPromptCheck->isChecked());
    accept();
}

void SettingsDialog::onRestoreDefaults()
{
    // 【修复】使用 Constants 命名空间填充默认值
    m_serverUrlEdit->setText(Constants::DEFAULT_SERVER_URL);
    m_scScreenshotEdit->setText(Constants::SHORTCUT_SCREENSHOT);
    m_scTextEdit->setText(Constants::SHORTCUT_TEXT);
    m_scFormulaEdit->setText(Constants::SHORTCUT_FORMULA);
    m_scTableEdit->setText(Constants::SHORTCUT_TABLE);
    m_autoUseLastPromptCheck->setChecked(true);
}
