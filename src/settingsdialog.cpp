#include "settingsdialog.h"
#include "constants.h"

#include <QJsonDocument>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QScrollArea>
#include <QFrame>
#include <QSplitter>
#include <QUuid>

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
    setupUi();
    loadSettings();
}

void SettingsDialog::setupUi()
{
    setWindowTitle("设置");
    resize(700, 750);

    QVBoxLayout* dialogLayout = new QVBoxLayout(this);
    dialogLayout->setContentsMargins(0, 0, 0, 0);
    dialogLayout->setSpacing(0);

    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget* scrollContent = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(scrollContent);
    mainLayout->setContentsMargins(20, 20, 20, 10);
    mainLayout->setSpacing(15);

    // --- 服务管理设置 ---
    QGroupBox* serviceGroup = new QGroupBox("识别服务管理");
    QVBoxLayout* serviceGroupLayout = new QVBoxLayout(serviceGroup);

    QHBoxLayout* modeLayout = new QHBoxLayout();
    modeLayout->addWidget(new QLabel("服务切换策略:"));
    m_serviceSwitchModeCombo = new QComboBox();
    m_serviceSwitchModeCombo->addItem("仅保留一个服务运行 (节省资源)", SettingsManager::SingleService);
    m_serviceSwitchModeCombo->addItem("允许多个服务并行运行 (切换快)", SettingsManager::ParallelServices);
    modeLayout->addWidget(m_serviceSwitchModeCombo);
    serviceGroupLayout->addLayout(modeLayout);

    QSplitter* serviceSplitter = new QSplitter(Qt::Horizontal);

    QWidget* listPanel = new QWidget();
    QVBoxLayout* listLayout = new QVBoxLayout(listPanel);
    listLayout->setContentsMargins(0,0,0,0);

    m_serviceListWidget = new QListWidget();
    listLayout->addWidget(m_serviceListWidget);

    QHBoxLayout* listBtnLayout = new QHBoxLayout();
    m_addServiceBtn = new QPushButton("添加");
    m_removeServiceBtn = new QPushButton("删除");
    QPushButton* setDefaultBtn = new QPushButton("设为默认启动"); // 【新增】
    listBtnLayout->addWidget(m_addServiceBtn);
    listBtnLayout->addWidget(m_removeServiceBtn);
    listBtnLayout->addWidget(setDefaultBtn);
    listLayout->addLayout(listBtnLayout);

    serviceSplitter->addWidget(listPanel);

    QWidget* editPanel = new QWidget();
    QFormLayout* editLayout = new QFormLayout(editPanel);
    editPanel->setMinimumWidth(300);

    m_serviceNameEdit = new QLineEdit();
    m_serviceCommandEdit = new QLineEdit();
    m_serviceUrlEdit = new QLineEdit();
    m_serviceTextPromptEdit = new QLineEdit();
    m_serviceFormulaPromptEdit = new QLineEdit();
    m_serviceTablePromptEdit = new QLineEdit();

    editLayout->addRow("服务名称:", m_serviceNameEdit);
    editLayout->addRow("启动命令:", m_serviceCommandEdit);
    editLayout->addRow("服务地址:", m_serviceUrlEdit);
    editLayout->addRow("文字提示词:", m_serviceTextPromptEdit);
    editLayout->addRow("公式提示词:", m_serviceFormulaPromptEdit);
    editLayout->addRow("表格提示词:", m_serviceTablePromptEdit);

    serviceSplitter->addWidget(editPanel);
    serviceSplitter->setStretchFactor(1, 2);

    serviceGroupLayout->addWidget(serviceSplitter);

    QHBoxLayout* idleLayout = new QHBoxLayout();
    idleLayout->addWidget(new QLabel("空闲自动关闭时间(分钟, -1为禁用):"));
    m_serviceIdleTimeoutSpin = new QSpinBox();
    m_serviceIdleTimeoutSpin->setRange(-1, 9999);
    idleLayout->addWidget(m_serviceIdleTimeoutSpin);
    serviceGroupLayout->addLayout(idleLayout);

    mainLayout->addWidget(serviceGroup);

    // --- 全局默认设置 ---
    QGroupBox* globalGroup = new QGroupBox("全局默认设置 (无服务选中时使用)");
    QFormLayout* globalForm = new QFormLayout(globalGroup);

    m_serverUrlEdit = new QLineEdit();
    globalForm->addRow("默认服务器地址:", m_serverUrlEdit);

    m_globalTextPromptEdit = new QLineEdit();
    m_globalFormulaPromptEdit = new QLineEdit();
    m_globalTablePromptEdit = new QLineEdit();
    globalForm->addRow("默认文字提示词:", m_globalTextPromptEdit);
    globalForm->addRow("默认公式提示词:", m_globalFormulaPromptEdit);
    globalForm->addRow("默认表格提示词:", m_globalTablePromptEdit);

    mainLayout->addWidget(globalGroup);

    // --- 快捷键设置 ---
    QGroupBox* shortcutGroup = new QGroupBox("快捷键设置");
    QFormLayout* shortcutLayout = new QFormLayout(shortcutGroup);
    m_scScreenshotEdit = new ShortcutEdit();
    m_scTextEdit = new ShortcutEdit();
    m_scFormulaEdit = new ShortcutEdit();
    m_scTableEdit = new ShortcutEdit();
    m_scExternalProcessEdit = new ShortcutEdit();

    shortcutLayout->addRow("截图:", m_scScreenshotEdit);
    shortcutLayout->addRow("文字识别:", m_scTextEdit);
    shortcutLayout->addRow("公式识别:", m_scFormulaEdit);
    shortcutLayout->addRow("表格识别:", m_scTableEdit);
    shortcutLayout->addRow("外部程序处理:", m_scExternalProcessEdit);
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

    m_rendererFontSpin = new QSpinBox();
    m_rendererFontSpin->setRange(8, 32);
    m_rendererFontSpin->setSuffix(" pt");
    displayLayout->addRow("渲染区字体大小:", m_rendererFontSpin);

    m_sourceEditorFontSpin = new QSpinBox();
    m_sourceEditorFontSpin->setRange(8, 32);
    m_sourceEditorFontSpin->setSuffix(" pt");
    displayLayout->addRow("源码编辑字体大小:", m_sourceEditorFontSpin);
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
    behaviorLayout->addWidget(m_autoExternalProcessCheck);

    // 【新增】自动启动服务开关
    m_autoStartServiceCheck = new QCheckBox("远程连接失败自动启动本地服务");
    m_autoStartServiceCheck->setToolTip("当使用全局默认设置连接失败时，尝试自动启动并切换到指定的默认本地服务。");
    behaviorLayout->addWidget(m_autoStartServiceCheck);

    mainLayout->addWidget(behaviorGroup);

    // --- 外部程序设置 ---
    QGroupBox* externalGroup = new QGroupBox("外部处理程序");
    QVBoxLayout* externalLayout = new QVBoxLayout(externalGroup);
    m_externalProcessorEdit = new QLineEdit();
    m_externalProcessorEdit->setPlaceholderText("例如: python3 /path/to/script.py");
    externalLayout->addWidget(m_externalProcessorEdit);
    mainLayout->addWidget(externalGroup);

    // --- 高级参数 ---
    QGroupBox* advancedGroup = new QGroupBox("高级请求参数");
    QVBoxLayout* advLayout = new QVBoxLayout(advancedGroup);
    m_requestParamsEdit = new QPlainTextEdit();
    QFont font("monospace");
    font.setStyleHint(QFont::Monospace);
    m_requestParamsEdit->setFont(font);
    m_requestParamsEdit->setMaximumHeight(100);
    advLayout->addWidget(m_requestParamsEdit);
    mainLayout->addWidget(advancedGroup);

    mainLayout->addStretch();

    scrollArea->setWidget(scrollContent);
    dialogLayout->addWidget(scrollArea);

    // --- 底部按钮 ---
    QWidget* bottomWidget = new QWidget();
    QVBoxLayout* bottomLayout = new QVBoxLayout(bottomWidget);
    bottomLayout->setContentsMargins(20, 10, 20, 20);
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::RestoreDefaults);

    // 【修复】禁止按钮获取 AutoDefault，防止回车误触发
    buttonBox->button(QDialogButtonBox::Save)->setAutoDefault(false);
    buttonBox->button(QDialogButtonBox::RestoreDefaults)->setAutoDefault(false);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::onSaveClicked);
    connect(buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this, &SettingsDialog::onRestoreDefaults);
    bottomLayout->addWidget(buttonBox);
    dialogLayout->addWidget(bottomWidget);

    // --- 连接信号 ---
    connect(m_addServiceBtn, &QPushButton::clicked, this, &SettingsDialog::onAddService);
    connect(m_removeServiceBtn, &QPushButton::clicked, this, &SettingsDialog::onRemoveService);

    // 列表选中项改变
    connect(m_serviceListWidget, &QListWidget::currentRowChanged, this, [this](int row){
        if (m_currentEditingServiceIndex != -1 && m_currentEditingServiceIndex < m_tempServiceProfiles.size()) {
            saveCurrentServiceToTemp(m_currentEditingServiceIndex);
        }

        m_currentEditingServiceIndex = row;

        if (row >= 0 && row < m_tempServiceProfiles.size()) {
            const auto& p = m_tempServiceProfiles[row];

            bool oldStateName = m_serviceNameEdit->blockSignals(true);
            bool oldStateCmd = m_serviceCommandEdit->blockSignals(true);
            bool oldStateUrl = m_serviceUrlEdit->blockSignals(true);
            bool oldStateText = m_serviceTextPromptEdit->blockSignals(true);
            bool oldStateFormula = m_serviceFormulaPromptEdit->blockSignals(true);
            bool oldStateTable = m_serviceTablePromptEdit->blockSignals(true);

            m_serviceNameEdit->setText(p.name);
            m_serviceCommandEdit->setText(p.startCommand);
            m_serviceUrlEdit->setText(p.serverUrl);
            m_serviceTextPromptEdit->setText(p.textPrompt);
            m_serviceFormulaPromptEdit->setText(p.formulaPrompt);
            m_serviceTablePromptEdit->setText(p.tablePrompt);

            m_serviceNameEdit->blockSignals(oldStateName);
            m_serviceCommandEdit->blockSignals(oldStateCmd);
            m_serviceUrlEdit->blockSignals(oldStateUrl);
            m_serviceTextPromptEdit->blockSignals(oldStateText);
            m_serviceFormulaPromptEdit->blockSignals(oldStateFormula);
            m_serviceTablePromptEdit->blockSignals(oldStateTable);

            if (m_serviceListWidget->item(row)) {
                m_serviceListWidget->item(row)->setText(p.name.isEmpty() ? tr("未命名服务") : p.name);
            }
        }
    });

    // 设为默认启动按钮
    connect(setDefaultBtn, &QPushButton::clicked, this, [this](){
        int row = m_serviceListWidget->currentRow();
        if (row >= 0 && row < m_tempServiceProfiles.size()) {
            QString id = m_tempServiceProfiles[row].id;
            SettingsManager::instance()->setDefaultLocalServiceId(id);
            QMessageBox::information(this, "提示", "已将 '" + m_tempServiceProfiles[row].name + "' 设为默认自动启动服务。\n当全局默认配置连接失败时，将自动切换到此服务。");
        }
    });

    connect(m_serviceNameEdit, &QLineEdit::textChanged, this, [this](const QString& text){
        if (m_currentEditingServiceIndex >= 0 && m_currentEditingServiceIndex < m_tempServiceProfiles.size()) {
            m_tempServiceProfiles[m_currentEditingServiceIndex].name = text;
            if (m_serviceListWidget->item(m_currentEditingServiceIndex)) {
                m_serviceListWidget->item(m_currentEditingServiceIndex)->setText(text.isEmpty() ? tr("未命名服务") : text);
            }
        }
    });

    connect(m_serviceCommandEdit, &QLineEdit::textChanged, this, &SettingsDialog::onServiceDetailChanged);
    connect(m_serviceUrlEdit, &QLineEdit::textChanged, this, &SettingsDialog::onServiceDetailChanged);
    connect(m_serviceTextPromptEdit, &QLineEdit::textChanged, this, &SettingsDialog::onServiceDetailChanged);
    connect(m_serviceFormulaPromptEdit, &QLineEdit::textChanged, this, &SettingsDialog::onServiceDetailChanged);
    connect(m_serviceTablePromptEdit, &QLineEdit::textChanged, this, &SettingsDialog::onServiceDetailChanged);
}

void SettingsDialog::loadSettings()
{
    SettingsManager* s = SettingsManager::instance();
    m_tempServiceProfiles = s->serviceProfiles();
    populateServiceList();

    m_serverUrlEdit->setText(s->serverUrl());
    m_globalTextPromptEdit->setText(s->textPrompt());
    m_globalFormulaPromptEdit->setText(s->formulaPrompt());
    m_globalTablePromptEdit->setText(s->tablePrompt());

    int mode = s->serviceSwitchMode();
    int modeIdx = m_serviceSwitchModeCombo->findData(mode);
    if (modeIdx != -1) m_serviceSwitchModeCombo->setCurrentIndex(modeIdx);

    m_serviceIdleTimeoutSpin->setValue(s->serviceIdleTimeout());

    m_scScreenshotEdit->setText(s->screenshotShortcut());
    m_scTextEdit->setText(s->textRecognizeShortcut());
    m_scFormulaEdit->setText(s->formulaRecognizeShortcut());
    m_scTableEdit->setText(s->tableRecognizeShortcut());
    m_scExternalProcessEdit->setText(s->externalProcessShortcut());

    m_autoUseLastPromptCheck->setChecked(s->autoUseLastPrompt());
    m_displayMathCombo->setCurrentIndex(m_displayMathCombo->findData(s->displayMathEnvironment()));
    m_mathFontCombo->setCurrentIndex(m_mathFontCombo->findData(s->mathFont()));
    m_externalProcessorEdit->setText(s->externalProcessorCommand());
    m_autoRecognizeCheck->setChecked(s->autoRecognizeOnScreenshot());
    m_autoCopyCheck->setChecked(s->autoCopyResult());
    m_requestParamsEdit->setPlainText(s->requestParameters());
    m_autoExternalProcessCheck->setChecked(s->autoExternalProcessBeforeCopy());
    m_rendererFontSpin->setValue(s->rendererFontSize());
    m_sourceEditorFontSpin->setValue(s->sourceEditorFontSize());

    // 【新增】加载自动启动设置
    m_autoStartServiceCheck->setChecked(s->autoStartService());
}

void SettingsDialog::populateServiceList()
{
    m_serviceListWidget->clear();
    for (const auto& p : m_tempServiceProfiles) {
        m_serviceListWidget->addItem(p.name.isEmpty() ? tr("未命名服务") : p.name);
    }
}

void SettingsDialog::saveCurrentServiceToTemp(int row)
{
    if (row < 0 || row >= m_tempServiceProfiles.size()) return;
    ServiceProfile& p = m_tempServiceProfiles[row];
    p.name = m_serviceNameEdit->text();
    p.startCommand = m_serviceCommandEdit->text();
    p.serverUrl = m_serviceUrlEdit->text();
    p.textPrompt = m_serviceTextPromptEdit->text();
    p.formulaPrompt = m_serviceFormulaPromptEdit->text();
    p.tablePrompt = m_serviceTablePromptEdit->text();
}

void SettingsDialog::onServiceDetailChanged()
{
    if (m_currentEditingServiceIndex != -1) {
        saveCurrentServiceToTemp(m_currentEditingServiceIndex);
    }
}

void SettingsDialog::onAddService()
{
    if (m_currentEditingServiceIndex != -1) {
        saveCurrentServiceToTemp(m_currentEditingServiceIndex);
    }

    ServiceProfile newProfile;
    newProfile.id = QUuid::createUuid().toString();
    newProfile.name = "新服务";
    newProfile.serverUrl = "http://localhost:8080/v1/chat/completions";
    newProfile.textPrompt = Constants::PROMPT_TEXT;
    newProfile.formulaPrompt = Constants::PROMPT_FORMULA;
    newProfile.tablePrompt = Constants::PROMPT_TABLE;

    m_tempServiceProfiles.append(newProfile);

    populateServiceList();

    m_serviceListWidget->setCurrentRow(m_tempServiceProfiles.size() - 1);
}

void SettingsDialog::onRemoveService()
{
    int row = m_serviceListWidget->currentRow();
    if (row < 0) return;

    m_tempServiceProfiles.removeAt(row);

    m_currentEditingServiceIndex = -1;

    populateServiceList();

    if (row < m_serviceListWidget->count()) {
        m_serviceListWidget->setCurrentRow(row);
    } else if (m_serviceListWidget->count() > 0) {
        m_serviceListWidget->setCurrentRow(m_serviceListWidget->count() - 1);
    }
}

void SettingsDialog::onSaveClicked()
{
    QString paramsText = m_requestParamsEdit->toPlainText().trimmed();
    if (!paramsText.isEmpty()) {
        QJsonParseError err;
        QJsonDocument::fromJson(paramsText.toUtf8(), &err);
        if (err.error != QJsonParseError::NoError) {
            QMessageBox::warning(this, "格式错误", "请求参数不是合法的 JSON 格式:\n" + err.errorString());
            return;
        }
    }

    if (m_currentEditingServiceIndex != -1) {
        saveCurrentServiceToTemp(m_currentEditingServiceIndex);
    }

    SettingsManager* s = SettingsManager::instance();
    s->setServiceProfiles(m_tempServiceProfiles);
    s->setServiceSwitchMode(static_cast<SettingsManager::ServiceSwitchMode>(m_serviceSwitchModeCombo->currentData().toInt()));

    s->setServerUrl(m_serverUrlEdit->text());
    s->setTextPrompt(m_globalTextPromptEdit->text());
    s->setFormulaPrompt(m_globalFormulaPromptEdit->text());
    s->setTablePrompt(m_globalTablePromptEdit->text());

    s->setServiceIdleTimeout(m_serviceIdleTimeoutSpin->value());
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
    s->setRequestParameters(paramsText);
    s->setAutoExternalProcessBeforeCopy(m_autoExternalProcessCheck->isChecked());
    s->setExternalProcessShortcut(m_scExternalProcessEdit->text());
    s->setRendererFontSize(m_rendererFontSpin->value());
    s->setSourceEditorFontSize(m_sourceEditorFontSpin->value());

    // 【新增】保存自动启动设置
    s->setAutoStartService(m_autoStartServiceCheck->isChecked());

    accept();
}

void SettingsDialog::onRestoreDefaults()
{
    m_serverUrlEdit->setText(Constants::DEFAULT_SERVER_URL);
    m_globalTextPromptEdit->setText(Constants::PROMPT_TEXT);
    m_globalFormulaPromptEdit->setText(Constants::PROMPT_FORMULA);
    m_globalTablePromptEdit->setText(Constants::PROMPT_TABLE);
    m_scScreenshotEdit->setText(Constants::SHORTCUT_SCREENSHOT);
    m_scTextEdit->setText(Constants::SHORTCUT_TEXT);
    m_scFormulaEdit->setText(Constants::SHORTCUT_FORMULA);
    m_scTableEdit->setText(Constants::SHORTCUT_TABLE);
    m_autoUseLastPromptCheck->setChecked(true);
    m_autoRecognizeCheck->setChecked(Constants::DEFAULT_AUTO_RECOGNIZE_SCREENSHOT);
    m_autoCopyCheck->setChecked(Constants::DEFAULT_AUTO_COPY_RESULT);
    m_displayMathCombo->setCurrentIndex(m_displayMathCombo->findData(Constants::DEFAULT_DISPLAY_MATH_ENV));
    m_mathFontCombo->setCurrentIndex(m_mathFontCombo->findData(Constants::DEFAULT_MATH_FONT));
    m_serviceIdleTimeoutSpin->setValue(Constants::DEFAULT_SERVICE_IDLE_TIMEOUT);
    m_requestParamsEdit->setPlainText(Constants::DEFAULT_REQUEST_PARAMETERS);
    m_autoExternalProcessCheck->setChecked(false);
    m_scExternalProcessEdit->setText(Constants::SHORTCUT_EXTERNAL_PROCESS);
    m_rendererFontSpin->setValue(Constants::DEFAULT_RENDERER_FONT_SIZE);
    m_sourceEditorFontSpin->setValue(Constants::DEFAULT_SOURCE_EDITOR_FONT_SIZE);
    m_externalProcessorEdit->clear();

    // 【新增】恢复默认自动启动设置
    m_autoStartServiceCheck->setChecked(Constants::DEFAULT_AUTO_START_SERVICE);
}
