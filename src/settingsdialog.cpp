#include "settingsdialog.h"
#include "settingsmanager.h"
#include "constants.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QLabel>

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
    setupUi();
    loadSettings();
}

void SettingsDialog::setupUi()
{
    setWindowTitle("设置");
    resize(450, 450);

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

    // --- 显示设置 ---
    QGroupBox* displayGroup = new QGroupBox("显示设置");
    QFormLayout* displayLayout = new QFormLayout(displayGroup);

    // 行间公式环境
    m_displayMathCombo = new QComboBox();
    m_displayMathCombo->addItem("$$ ... $$", "$$");
    m_displayMathCombo->addItem("\\begin{equation} ... \\end{equation}", "equation");
    m_displayMathCombo->addItem("\\begin{align} ... \\end{align}", "align");
    displayLayout->addRow("行间公式环境:", m_displayMathCombo);

    // 【新增】数学字体选择
    m_mathFontCombo = new QComboBox();
    // 注意：此处 UserData 必须与资源文件名后缀完全一致
    m_mathFontCombo->addItem("Latin Modern", "Latin-Modern");
    m_mathFontCombo->addItem("Asana", "Asana");
    m_mathFontCombo->addItem("STIX Two", "STIX2");
    m_mathFontCombo->addItem("Libertinus", "Libertinus");
    m_mathFontCombo->addItem("Noto Sans", "NotoSans");
    m_mathFontCombo->addItem("Local (系统默认)", "Local");
    displayLayout->addRow("数学字体:", m_mathFontCombo);

    mainLayout->addWidget(displayGroup);

    // --- 行为设置 ---
    QGroupBox* behaviorGroup = new QGroupBox("行为设置");
    QVBoxLayout* behaviorLayout = new QVBoxLayout(behaviorGroup);
    m_autoUseLastPromptCheck = new QCheckBox("自动使用上次识别类型");
    behaviorLayout->addWidget(m_autoUseLastPromptCheck);
    m_autoRecognizeCheck = new QCheckBox("截图后自动识别");
    behaviorLayout->addWidget(m_autoRecognizeCheck);
    m_autoCopyCheck = new QCheckBox("识别完成后自动复制结果");
    behaviorLayout->addWidget(m_autoCopyCheck);
    mainLayout->addWidget(behaviorGroup);

    // --- 外部程序设置 ---
    QGroupBox* externalGroup = new QGroupBox("外部处理程序");
    QVBoxLayout* externalLayout = new QVBoxLayout(externalGroup);
    QLabel* extHint = new QLabel("命令接收 Markdown 文本作为标准输入，\n并将处理后的文本输出到标准输出。");
    extHint->setStyleSheet("color: gray; font-size: 11px;");
    externalLayout->addWidget(extHint);
    m_externalProcessorEdit = new QLineEdit();
    m_externalProcessorEdit->setPlaceholderText("例如: python3 /path/to/script.py");
    externalLayout->addWidget(m_externalProcessorEdit);
    mainLayout->addWidget(externalGroup);

    // --- 按钮 ---
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::RestoreDefaults);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::onSaveClicked);
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

    // 加载行间公式环境
    int envIndex = m_displayMathCombo->findData(s->displayMathEnvironment());
    if (envIndex != -1) m_displayMathCombo->setCurrentIndex(envIndex);

    // 【新增】加载字体设置
    int fontIndex = m_mathFontCombo->findData(s->mathFont());
    if (fontIndex != -1) m_mathFontCombo->setCurrentIndex(fontIndex);

    m_externalProcessorEdit->setText(s->externalProcessorCommand());
    m_autoRecognizeCheck->setChecked(s->autoRecognizeOnScreenshot());
    m_autoCopyCheck->setChecked(s->autoCopyResult());
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
    s->setDisplayMathEnvironment(m_displayMathCombo->currentData().toString());

    // 【新增】保存字体设置
    s->setMathFont(m_mathFontCombo->currentData().toString());

    s->setExternalProcessorCommand(m_externalProcessorEdit->text());
    s->setAutoRecognizeOnScreenshot(m_autoRecognizeCheck->isChecked());
    s->setAutoCopyResult(m_autoCopyCheck->isChecked());

    accept();
}

void SettingsDialog::onRestoreDefaults()
{
    m_serverUrlEdit->setText(Constants::DEFAULT_SERVER_URL);
    m_scScreenshotEdit->setText(Constants::SHORTCUT_SCREENSHOT);
    m_scTextEdit->setText(Constants::SHORTCUT_TEXT);
    m_scFormulaEdit->setText(Constants::SHORTCUT_FORMULA);
    m_scTableEdit->setText(Constants::SHORTCUT_TABLE);
    m_autoUseLastPromptCheck->setChecked(true);
    m_autoRecognizeCheck->setChecked(Constants::DEFAULT_AUTO_RECOGNIZE_SCREENSHOT);
    m_autoCopyCheck->setChecked(Constants::DEFAULT_AUTO_COPY_RESULT);

    int envIndex = m_displayMathCombo->findData(Constants::DEFAULT_DISPLAY_MATH_ENV);
    if (envIndex != -1) m_displayMathCombo->setCurrentIndex(envIndex);

    // 【新增】恢复默认字体
    int fontIndex = m_mathFontCombo->findData(Constants::DEFAULT_MATH_FONT);
    if (fontIndex != -1) m_mathFontCombo->setCurrentIndex(fontIndex);

    m_externalProcessorEdit->clear();
}
