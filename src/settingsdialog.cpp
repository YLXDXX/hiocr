#include "settingsdialog.h"
#include "settingsmanager.h"
#include "constants.h"

#include <QJsonDocument>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QSpinBox>
#include <QScrollArea>
#include <QFrame>


SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
    setupUi();
    loadSettings();
}

void SettingsDialog::setupUi()
{
    setWindowTitle("设置");
    // 【修改】稍微增加默认高度，宽度保持不变
    resize(500, 650);

    // --- 主布局：包含滚动区域和底部按钮 ---
    QVBoxLayout* dialogLayout = new QVBoxLayout(this);
    dialogLayout->setContentsMargins(0, 0, 0, 0); // 去除外边框，让滚动条看起来更自然
    dialogLayout->setSpacing(0);

    // --- 1. 创建滚动区域 ---
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true); // 关键：允许内部控件自动调整宽度
    scrollArea->setFrameShape(QFrame::NoFrame); // 去掉滚动区域的边框
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 禁用横向滚动

    // 创建滚动区域的内容容器
    QWidget* scrollContent = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(scrollContent);
    mainLayout->setContentsMargins(20, 20, 20, 10); // 设置内容区域的边距
    mainLayout->setSpacing(15); // 设置各个 GroupBox 之间的间距

    // ========== 以下为原有的设置项代码 (布局在 mainLayout 中) ==========

    // --- 服务器设置 ---
    QGroupBox* serverGroup = new QGroupBox("服务器设置");
    QFormLayout* serverLayout = new QFormLayout(serverGroup);
    m_serverUrlEdit = new QLineEdit();
    serverLayout->addRow("服务器地址:", m_serverUrlEdit);
    mainLayout->addWidget(serverGroup);

    // --- 提示词设置 ---
    QGroupBox* promptGroup = new QGroupBox("提示词设置");
    QFormLayout* promptLayout = new QFormLayout(promptGroup);

    m_textPromptEdit = new QLineEdit();
    m_formulaPromptEdit = new QLineEdit();
    m_tablePromptEdit = new QLineEdit();

    promptLayout->addRow("文字识别:", m_textPromptEdit);
    promptLayout->addRow("公式识别:", m_formulaPromptEdit);
    promptLayout->addRow("表格识别:", m_tablePromptEdit);

    mainLayout->addWidget(promptGroup);

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

    m_displayMathCombo = new QComboBox();
    m_displayMathCombo->addItem("$$ ... $$", "$$");
    m_displayMathCombo->addItem("\\begin{equation} ... \\end{equation}", "equation");
    m_displayMathCombo->addItem("\\begin{align} ... \\end{align}", "align");
    displayLayout->addRow("行间公式环境:", m_displayMathCombo);

    m_mathFontCombo = new QComboBox();
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
    m_autoExternalProcessCheck = new QCheckBox("复制前自动调用外部程序处理");
    m_autoExternalProcessCheck->setToolTip("开启后，点击复制按钮时将自动运行外部程序处理文本后再复制。");
    behaviorLayout->addWidget(m_autoExternalProcessCheck);
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

    // --- 高级参数设置 ---
    QGroupBox* advancedGroup = new QGroupBox("高级请求参数");
    QVBoxLayout* advLayout = new QVBoxLayout(advancedGroup);

    QLabel* advHint = new QLabel("直接编辑 JSON 对象，将合并到请求体中。\n支持参数: temperature, max_tokens, top_p 等。");
    advHint->setStyleSheet("color: gray; font-size: 11px;");
    advHint->setWordWrap(true);

    m_requestParamsEdit = new QPlainTextEdit();
    QFont font("monospace");
    font.setStyleHint(QFont::Monospace);
    m_requestParamsEdit->setFont(font);
    m_requestParamsEdit->setPlaceholderText("{\"temperature\": 0.7}");
    // 限制高度，避免 JSON 编辑框占用过多空间
    m_requestParamsEdit->setMaximumHeight(100);

    advLayout->addWidget(advHint);
    advLayout->addWidget(m_requestParamsEdit);

    mainLayout->addWidget(advancedGroup);

    // --- 服务管理设置 ---
    QGroupBox* serviceGroup = new QGroupBox("服务管理");
    QVBoxLayout* serviceLayout = new QVBoxLayout(serviceGroup);

    m_autoStartServiceCheck = new QCheckBox("自动启动本地识别服务");
    serviceLayout->addWidget(m_autoStartServiceCheck);

    QHBoxLayout* cmdLayout = new QHBoxLayout();
    cmdLayout->addWidget(new QLabel("启动命令:"));
    m_serviceStartCommandEdit = new QLineEdit();
    m_serviceStartCommandEdit->setPlaceholderText("例如: python server.py --port 8080");
    cmdLayout->addWidget(m_serviceStartCommandEdit);
    serviceLayout->addLayout(cmdLayout);

    QHBoxLayout* timeoutLayout = new QHBoxLayout();
    timeoutLayout->addWidget(new QLabel("空闲自动关闭时间(分钟, -1为禁用):"));
    m_serviceIdleTimeoutSpin = new QSpinBox();
    m_serviceIdleTimeoutSpin->setRange(-1, 9999);
    timeoutLayout->addWidget(m_serviceIdleTimeoutSpin);
    serviceLayout->addLayout(timeoutLayout);

    mainLayout->addWidget(serviceGroup);

    // 添加一个垂直弹簧，将所有设置项顶上去
    mainLayout->addStretch();

    // --- 2. 将内容容器设置给滚动区域 ---
    scrollArea->setWidget(scrollContent);

    // --- 3. 将滚动区域添加到对话框主布局 ---
    dialogLayout->addWidget(scrollArea);

    // --- 4. 底部按钮区域 (固定在底部) ---
    QWidget* bottomWidget = new QWidget();
    QVBoxLayout* bottomLayout = new QVBoxLayout(bottomWidget);
    bottomLayout->setContentsMargins(20, 10, 20, 20); // 左、上、右、下边距

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::RestoreDefaults);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::onSaveClicked);
    QPushButton* restoreBtn = buttonBox->button(QDialogButtonBox::RestoreDefaults);
    if (restoreBtn) {
        connect(restoreBtn, &QPushButton::clicked, this, &SettingsDialog::onRestoreDefaults);
    }
    bottomLayout->addWidget(buttonBox);

    // 分隔线（可选，视觉上区分内容和按钮）
    QFrame* separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    // 在布局中先加分隔线再加按钮，或者加在 top/bottom margin 里
    // 这里我们把它加在按钮上方
    bottomLayout->insertWidget(0, separator);

    dialogLayout->addWidget(bottomWidget);
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

    m_autoStartServiceCheck->setChecked(s->autoStartService());
    m_serviceStartCommandEdit->setText(s->serviceStartCommand());
    m_serviceIdleTimeoutSpin->setValue(s->serviceIdleTimeout());

    m_requestParamsEdit->setPlainText(SettingsManager::instance()->requestParameters());

    // 加载提示词设置
    m_textPromptEdit->setText(s->textPrompt());
    m_formulaPromptEdit->setText(s->formulaPrompt());
    m_tablePromptEdit->setText(s->tablePrompt());

    m_autoExternalProcessCheck->setChecked(s->autoExternalProcessBeforeCopy());
}

void SettingsDialog::onSaveClicked()
{
    // 校验 JSON 格式
    QString paramsText = m_requestParamsEdit->toPlainText().trimmed();
    if (!paramsText.isEmpty()) {
        QJsonParseError err;
        QJsonDocument::fromJson(paramsText.toUtf8(), &err);
        if (err.error != QJsonParseError::NoError) {
            QMessageBox::warning(this, "格式错误", "请求参数不是合法的 JSON 格式:\n" + err.errorString());
            return; // 阻止保存
        }
    }

    SettingsManager* s = SettingsManager::instance();
    s->setServerUrl(m_serverUrlEdit->text());
    s->setScreenshotShortcut(m_scScreenshotEdit->text());
    s->setTextRecognizeShortcut(m_scTextEdit->text());
    s->setFormulaRecognizeShortcut(m_scFormulaEdit->text());
    s->setTableRecognizeShortcut(m_scTableEdit->text());
    s->setAutoUseLastPrompt(m_autoUseLastPromptCheck->isChecked());
    s->setDisplayMathEnvironment(m_displayMathCombo->currentData().toString());
    s->setMathFont(m_mathFontCombo->currentData().toString());
    s->setExternalProcessorCommand(m_externalProcessorEdit->text());
    s->setAutoRecognizeOnScreenshot(m_autoRecognizeCheck->isChecked());
    s->setAutoCopyResult(m_autoCopyCheck->isChecked());
    s->setAutoStartService(m_autoStartServiceCheck->isChecked());
    s->setServiceStartCommand(m_serviceStartCommandEdit->text());
    s->setServiceIdleTimeout(m_serviceIdleTimeoutSpin->value());

    // 保存请求参数
    s->setRequestParameters(paramsText);

    // 保存提示词设置
    s->setTextPrompt(m_textPromptEdit->text());
    s->setFormulaPrompt(m_formulaPromptEdit->text());
    s->setTablePrompt(m_tablePromptEdit->text());

    s->setAutoExternalProcessBeforeCopy(m_autoExternalProcessCheck->isChecked());

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

    m_autoStartServiceCheck->setChecked(Constants::DEFAULT_AUTO_START_SERVICE);
    m_serviceStartCommandEdit->setText(Constants::DEFAULT_SERVICE_START_COMMAND);
    m_serviceIdleTimeoutSpin->setValue(Constants::DEFAULT_SERVICE_IDLE_TIMEOUT);

    m_requestParamsEdit->setPlainText(Constants::DEFAULT_REQUEST_PARAMETERS);

    m_textPromptEdit->setText(Constants::PROMPT_TEXT);
    m_formulaPromptEdit->setText(Constants::PROMPT_FORMULA);
    m_tablePromptEdit->setText(Constants::PROMPT_TABLE);

    m_autoExternalProcessCheck->setChecked(false); // 默认关闭

    m_externalProcessorEdit->clear();
}
