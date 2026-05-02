#ifndef SETTINGSDIALOG_IMPL_H
#define SETTINGSDIALOG_IMPL_H

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

inline SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
    setupUi();
    loadSettings();
}

inline void SettingsDialog::setupUi()
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

    // 【新增】
    m_serviceApiKeyEdit = new QLineEdit();
    m_serviceApiKeyEdit->setEchoMode(QLineEdit::Password); // 密码模式
    m_serviceModelEdit = new QLineEdit();
    m_serviceModelEdit->setPlaceholderText("例如: qwen-vl-plus, deepseek-chat");

    editLayout->addRow("服务名称:", m_serviceNameEdit);
    editLayout->addRow("启动命令:", m_serviceCommandEdit);
    editLayout->addRow("服务地址:", m_serviceUrlEdit);
    editLayout->addRow("API Key:", m_serviceApiKeyEdit);     // 【新增】
    editLayout->addRow("模型名称:", m_serviceModelEdit);     // 【新增】
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

    // 【新增】
    m_globalApiKeyEdit = new QLineEdit();
    m_globalApiKeyEdit->setEchoMode(QLineEdit::Password);
    globalForm->addRow("API Key:", m_globalApiKeyEdit);

    // 【新增】
    m_globalModelNameEdit = new QLineEdit();
    m_globalModelNameEdit->setPlaceholderText("例如: qwen-plus, deepseek-chat");
    globalForm->addRow("模型名称:", m_globalModelNameEdit);

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
    m_abortScEdit = new ShortcutEdit();

    shortcutLayout->addRow("截图:", m_scScreenshotEdit);
    shortcutLayout->addRow("文字识别:", m_scTextEdit);
    shortcutLayout->addRow("公式识别:", m_scFormulaEdit);
    shortcutLayout->addRow("表格识别:", m_scTableEdit);
    shortcutLayout->addRow("强制中断识别:", m_abortScEdit);
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

    // --- 静默模式设置 ---
    QHBoxLayout* silentLayout = new QHBoxLayout();
    m_silentModeCheck = new QCheckBox("启用静默模式");
    silentLayout->addWidget(m_silentModeCheck);
    silentLayout->addStretch();
    silentLayout->addWidget(new QLabel("通知方式:"));
    m_silentModeNotificationCombo = new QComboBox();
    m_silentModeNotificationCombo->addItem("系统通知", "system_notification");
    m_silentModeNotificationCombo->addItem("悬浮小球", "floating_ball");
    silentLayout->addWidget(m_silentModeNotificationCombo);
    behaviorLayout->addLayout(silentLayout);

    // 静默模式复选框控制通知方式可用性
    connect(m_silentModeCheck, &QCheckBox::toggled, this, [this](bool checked){
        m_silentModeNotificationCombo->setEnabled(checked);
    });

    // --- 悬浮球设置 ---
    QGroupBox* floatingBallGroup = new QGroupBox("悬浮球设置");
    QFormLayout* fbLayout = new QFormLayout(floatingBallGroup);

    m_floatingBallSizeSpin = new QSpinBox();
    m_floatingBallSizeSpin->setRange(32, 128);
    m_floatingBallSizeSpin->setSuffix(" px");
    fbLayout->addRow("悬浮球大小:", m_floatingBallSizeSpin);

    m_floatingBallAutoHideTimeSpin = new QSpinBox();
    m_floatingBallAutoHideTimeSpin->setRange(0, 60000);
    m_floatingBallAutoHideTimeSpin->setSingleStep(1000);
    m_floatingBallAutoHideTimeSpin->setSuffix(" ms");
    m_floatingBallAutoHideTimeSpin->setSpecialValueText("不自动隐藏 (0)");
    fbLayout->addRow("完成/错误后显示时间:", m_floatingBallAutoHideTimeSpin);

    m_floatingBallAlwaysVisibleCheck = new QCheckBox("始终显示悬浮球 (右键点击截图, 左键拖动)");
    fbLayout->addRow(m_floatingBallAlwaysVisibleCheck);

    behaviorLayout->addWidget(floatingBallGroup);

    QGroupBox* formatterGroup = new QGroupBox("LaTeX代码格式化工具");
    QFormLayout* fmLayout = new QFormLayout(formatterGroup);

    m_formatterEnabledCheck = new QCheckBox("启用LaTeX代码格式化");
    fmLayout->addRow(m_formatterEnabledCheck);

    m_formatterCommandEdit = new QLineEdit();
    m_formatterCommandEdit->setPlaceholderText("例如: latexindent - 或其他格式化命令");
    fmLayout->addRow("格式化命令:", m_formatterCommandEdit);

    m_formatterOrderCombo = new QComboBox();
    m_formatterOrderCombo->addItem("先格式化再外部处理", SettingsManager::FormatFirst);
    m_formatterOrderCombo->addItem("先外部处理再格式化", SettingsManager::ProcessFirst);
    fmLayout->addRow("执行顺序:", m_formatterOrderCombo);

    connect(m_formatterEnabledCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_formatterCommandEdit->setEnabled(checked);
        m_formatterOrderCombo->setEnabled(checked);
    });

    behaviorLayout->addWidget(formatterGroup);

    // 【新增】历史记录设置区域
    QHBoxLayout* historyLayout = new QHBoxLayout();
    m_saveHistoryCheck = new QCheckBox("保存识别历史记录");
    connect(m_saveHistoryCheck, &QCheckBox::toggled, this, [this](bool checked){
        m_historyLimitSpin->setEnabled(checked);
    });

    QLabel* limitLabel = new QLabel("最多保留条数:");
    m_historyLimitSpin = new QSpinBox();
    m_historyLimitSpin->setRange(0, 9999);
    m_historyLimitSpin->setSuffix(" 条");
    m_historyLimitSpin->setSpecialValueText("无限制"); // 0 表示无限制

    historyLayout->addWidget(m_saveHistoryCheck);
    historyLayout->addStretch();
    historyLayout->addWidget(limitLabel);
    historyLayout->addWidget(m_historyLimitSpin);
    behaviorLayout->addLayout(historyLayout);

    mainLayout->addWidget(behaviorGroup);

    // 【新增】自动启动服务开关
    m_autoStartServiceCheck = new QCheckBox("远程连接失败自动启动本地服务");
    m_autoStartServiceCheck->setToolTip("当使用全局默认设置连接失败时，尝试自动启动并切换到指定的默认本地服务。");
    behaviorLayout->addWidget(m_autoStartServiceCheck);

    mainLayout->addWidget(behaviorGroup);

    // --- 新增：复制前外部程序处理设置 ---
    QGroupBox* processorGroup = new QGroupBox("复制前外部程序处理");
    QVBoxLayout* processorLayout = new QVBoxLayout(processorGroup);

    // 说明文字
    QLabel* procHint = new QLabel("配置不同类型内容在复制前调用的外部脚本。\n脚本从标准输入读取文本，处理后将结果输出到标准输出。");
    procHint->setWordWrap(true);
    processorLayout->addWidget(procHint);

    // 文字处理
    QGroupBox* textGroup = new QGroupBox("文字处理");
    QFormLayout* textLayout = new QFormLayout(textGroup);
    m_textProcessorEdit = new QLineEdit();
    m_textProcessorScEdit = new ShortcutEdit();
    textLayout->addRow("命令:", m_textProcessorEdit);
    textLayout->addRow("快捷键:", m_textProcessorScEdit);
    processorLayout->addWidget(textGroup);

    // 公式处理
    QGroupBox* formulaGroup = new QGroupBox("公式处理");
    QFormLayout* formulaLayout = new QFormLayout(formulaGroup);
    m_formulaProcessorEdit = new QLineEdit();
    m_formulaProcessorScEdit = new ShortcutEdit();
    formulaLayout->addRow("命令:", m_formulaProcessorEdit);
    formulaLayout->addRow("快捷键:", m_formulaProcessorScEdit);
    processorLayout->addWidget(formulaGroup);

    // 表格处理
    QGroupBox* tableGroup = new QGroupBox("表格处理");
    QFormLayout* tableLayout = new QFormLayout(tableGroup);
    m_tableProcessorEdit = new QLineEdit();
    m_tableProcessorScEdit = new ShortcutEdit();
    tableLayout->addRow("命令:", m_tableProcessorEdit);
    tableLayout->addRow("快捷键:", m_tableProcessorScEdit);
    processorLayout->addWidget(tableGroup);

    // 纯数学公式处理
    QGroupBox* pureMathGroup = new QGroupBox("纯数学公式处理 (优先级最高)");
    QFormLayout* pureMathLayout = new QFormLayout(pureMathGroup);
    m_pureMathProcessorEdit = new QLineEdit();
    m_pureMathProcessorScEdit = new ShortcutEdit();
    pureMathLayout->addRow("命令:", m_pureMathProcessorEdit);
    pureMathLayout->addRow("快捷键:", m_pureMathProcessorScEdit);
    processorLayout->addWidget(pureMathGroup);

    mainLayout->addWidget(processorGroup);

    // --- 高级参数 ---
    QGroupBox* advancedGroup = new QGroupBox("高级请求参数");
    QVBoxLayout* advLayout = new QVBoxLayout(advancedGroup);

    // 【新增】超时设置布局
    QHBoxLayout* timeoutLayout = new QHBoxLayout();
    timeoutLayout->addWidget(new QLabel("请求超时时间 (秒):"));
    m_timeoutSpin = new QSpinBox();
    m_timeoutSpin->setRange(5, 300); // 限制范围 5秒 - 5分钟
    m_timeoutSpin->setSuffix(" 秒");
    timeoutLayout->addWidget(m_timeoutSpin);
    timeoutLayout->addStretch();
    advLayout->addLayout(timeoutLayout);

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
            m_serviceApiKeyEdit->setText(p.apiKey);
            m_serviceModelEdit->setText(p.modelName);
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

inline void SettingsDialog::loadSettings()
{
    SettingsManager* s = SettingsManager::instance();
    m_tempServiceProfiles = s->serviceProfiles();
    populateServiceList();

    m_serverUrlEdit->setText(s->serverUrl());
    m_globalApiKeyEdit->setText(s->globalApiKey());
    m_globalModelNameEdit->setText(s->globalModelName());
    m_globalTextPromptEdit->setText(s->textPrompt());
    m_globalFormulaPromptEdit->setText(s->formulaPrompt());
    m_globalTablePromptEdit->setText(s->tablePrompt());

    int mode = s->serviceSwitchMode();
    int modeIdx = m_serviceSwitchModeCombo->findData(mode);
    if (modeIdx != -1) m_serviceSwitchModeCombo->setCurrentIndex(modeIdx);

    m_serviceIdleTimeoutSpin->setValue(s->serviceIdleTimeout());

    // 加载识别快捷键
    m_scScreenshotEdit->setText(s->screenshotShortcut());
    m_scTextEdit->setText(s->textRecognizeShortcut());
    m_scFormulaEdit->setText(s->formulaRecognizeShortcut());
    m_scTableEdit->setText(s->tableRecognizeShortcut());
    m_abortScEdit->setText(s->abortShortcut());

    m_autoUseLastPromptCheck->setChecked(s->autoUseLastPrompt());
    m_displayMathCombo->setCurrentIndex(m_displayMathCombo->findData(s->displayMathEnvironment()));
    m_mathFontCombo->setCurrentIndex(m_mathFontCombo->findData(s->mathFont()));

    m_autoRecognizeCheck->setChecked(s->autoRecognizeOnScreenshot());
    m_autoCopyCheck->setChecked(s->autoCopyResult());
    m_timeoutSpin->setValue(s->requestTimeout());
    m_requestParamsEdit->setPlainText(s->requestParameters());
    m_autoExternalProcessCheck->setChecked(s->autoExternalProcessBeforeCopy());

    // 【新增】加载分类型脚本配置
    m_textProcessorEdit->setText(s->textProcessorCommand());
    m_textProcessorScEdit->setText(s->textProcessorShortcut());

    m_formulaProcessorEdit->setText(s->formulaProcessorCommand());
    m_formulaProcessorScEdit->setText(s->formulaProcessorShortcut());

    m_tableProcessorEdit->setText(s->tableProcessorCommand());
    m_tableProcessorScEdit->setText(s->tableProcessorShortcut());

    m_pureMathProcessorEdit->setText(s->pureMathProcessorCommand());
    m_pureMathProcessorScEdit->setText(s->pureMathProcessorShortcut());

    m_rendererFontSpin->setValue(s->rendererFontSize());
    m_sourceEditorFontSpin->setValue(s->sourceEditorFontSize());

    m_autoStartServiceCheck->setChecked(s->autoStartService());

    // 【新增】加载历史设置
    m_saveHistoryCheck->setChecked(s->saveHistoryEnabled());
    m_historyLimitSpin->setValue(s->historyLimit());
    m_historyLimitSpin->setEnabled(s->saveHistoryEnabled());

    m_silentModeCheck->setChecked(s->silentModeEnabled());
    m_silentModeNotificationCombo->setCurrentIndex(
        m_silentModeNotificationCombo->findData(s->silentModeNotificationType()));
    m_silentModeNotificationCombo->setEnabled(s->silentModeEnabled());

    m_floatingBallSizeSpin->setValue(s->floatingBallSize());
    m_floatingBallAutoHideTimeSpin->setValue(s->floatingBallAutoHideTime());
    m_floatingBallAlwaysVisibleCheck->setChecked(s->floatingBallAlwaysVisible());

    m_formatterEnabledCheck->setChecked(s->formatterEnabled());
    m_formatterCommandEdit->setText(s->formatterCommand());
    m_formatterOrderCombo->setCurrentIndex(
        m_formatterOrderCombo->findData(static_cast<int>(s->formatterOrder())));
    bool fmtEnabled = s->formatterEnabled();
    m_formatterCommandEdit->setEnabled(fmtEnabled);
    m_formatterOrderCombo->setEnabled(fmtEnabled);
}

inline void SettingsDialog::populateServiceList()
{
    m_serviceListWidget->clear();
    for (const auto& p : m_tempServiceProfiles) {
        m_serviceListWidget->addItem(p.name.isEmpty() ? tr("未命名服务") : p.name);
    }
}

inline void SettingsDialog::saveCurrentServiceToTemp(int row)
{
    if (row < 0 || row >= m_tempServiceProfiles.size()) return;
    ServiceProfile& p = m_tempServiceProfiles[row];
    p.name = m_serviceNameEdit->text();
    p.startCommand = m_serviceCommandEdit->text();
    p.serverUrl = m_serviceUrlEdit->text();
    p.apiKey = m_serviceApiKeyEdit->text();
    p.modelName = m_serviceModelEdit->text();
    p.textPrompt = m_serviceTextPromptEdit->text();
    p.formulaPrompt = m_serviceFormulaPromptEdit->text();
    p.tablePrompt = m_serviceTablePromptEdit->text();
}

inline void SettingsDialog::onServiceDetailChanged()
{
    if (m_currentEditingServiceIndex != -1) {
        saveCurrentServiceToTemp(m_currentEditingServiceIndex);
    }
}

inline void SettingsDialog::onAddService()
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

inline void SettingsDialog::onRemoveService()
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

inline void SettingsDialog::onSaveClicked()
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
    s->setGlobalApiKey(m_globalApiKeyEdit->text());
    s->setGlobalModelName(m_globalModelNameEdit->text());
    s->setTextPrompt(m_globalTextPromptEdit->text());
    s->setFormulaPrompt(m_globalFormulaPromptEdit->text());
    s->setTablePrompt(m_globalTablePromptEdit->text());

    s->setServiceIdleTimeout(m_serviceIdleTimeoutSpin->value());
    s->setScreenshotShortcut(m_scScreenshotEdit->text());
    s->setTextRecognizeShortcut(m_scTextEdit->text());
    s->setFormulaRecognizeShortcut(m_scFormulaEdit->text());
    s->setTableRecognizeShortcut(m_scTableEdit->text());
    s->setAbortShortcut(m_abortScEdit->text());
    s->setAutoUseLastPrompt(m_autoUseLastPromptCheck->isChecked());
    s->setDisplayMathEnvironment(m_displayMathCombo->currentData().toString());
    s->setMathFont(m_mathFontCombo->currentData().toString());

    s->setAutoRecognizeOnScreenshot(m_autoRecognizeCheck->isChecked());
    s->setAutoCopyResult(m_autoCopyCheck->isChecked());
    s->setRequestTimeout(m_timeoutSpin->value());
    s->setRequestParameters(paramsText);
    s->setAutoExternalProcessBeforeCopy(m_autoExternalProcessCheck->isChecked());

    // 分类型脚本配置
    s->setTextProcessorCommand(m_textProcessorEdit->text());
    s->setTextProcessorShortcut(m_textProcessorScEdit->text());

    s->setFormulaProcessorCommand(m_formulaProcessorEdit->text());
    s->setFormulaProcessorShortcut(m_formulaProcessorScEdit->text());

    s->setTableProcessorCommand(m_tableProcessorEdit->text());
    s->setTableProcessorShortcut(m_tableProcessorScEdit->text());

    s->setPureMathProcessorCommand(m_pureMathProcessorEdit->text());
    s->setPureMathProcessorShortcut(m_pureMathProcessorScEdit->text());

    s->setRendererFontSize(m_rendererFontSpin->value());
    s->setSourceEditorFontSize(m_sourceEditorFontSpin->value());

    s->setAutoStartService(m_autoStartServiceCheck->isChecked());

    // 历史设置
    s->setSaveHistoryEnabled(m_saveHistoryCheck->isChecked());
    s->setHistoryLimit(m_historyLimitSpin->value());

    s->setSilentModeEnabled(m_silentModeCheck->isChecked());
    s->setSilentModeNotificationType(m_silentModeNotificationCombo->currentData().toString());

    s->setFloatingBallSize(m_floatingBallSizeSpin->value());
    s->setFloatingBallAutoHideTime(m_floatingBallAutoHideTimeSpin->value());
    s->setFloatingBallAlwaysVisible(m_floatingBallAlwaysVisibleCheck->isChecked());

    s->setFormatterEnabled(m_formatterEnabledCheck->isChecked());
    s->setFormatterCommand(m_formatterCommandEdit->text());
    s->setFormatterOrder(static_cast<SettingsManager::FormatterOrder>(m_formatterOrderCombo->currentData().toInt()));

    // 【修复】显式同步到磁盘，防止程序异常退出时设置丢失
    // QSettings 的 setValue 只是写入内存缓存，直到析构才自动同步
    s->sync();

    accept();
}

inline void SettingsDialog::onRestoreDefaults()
{
    m_serverUrlEdit->setText(Constants::DEFAULT_SERVER_URL);
    m_globalTextPromptEdit->setText(Constants::PROMPT_TEXT);
    m_globalFormulaPromptEdit->setText(Constants::PROMPT_FORMULA);
    m_globalTablePromptEdit->setText(Constants::PROMPT_TABLE);
    m_scScreenshotEdit->setText(Constants::SHORTCUT_SCREENSHOT);
    m_scTextEdit->setText(Constants::SHORTCUT_TEXT);
    m_scFormulaEdit->setText(Constants::SHORTCUT_FORMULA);
    m_scTableEdit->setText(Constants::SHORTCUT_TABLE);
    m_abortScEdit->setText(Constants::SHORTCUT_ABORT);
    m_autoUseLastPromptCheck->setChecked(true);
    m_autoRecognizeCheck->setChecked(Constants::DEFAULT_AUTO_RECOGNIZE_SCREENSHOT);
    m_autoCopyCheck->setChecked(Constants::DEFAULT_AUTO_COPY_RESULT);
    m_displayMathCombo->setCurrentIndex(m_displayMathCombo->findData(Constants::DEFAULT_DISPLAY_MATH_ENV));
    m_mathFontCombo->setCurrentIndex(m_mathFontCombo->findData(Constants::DEFAULT_MATH_FONT));
    m_serviceIdleTimeoutSpin->setValue(Constants::DEFAULT_SERVICE_IDLE_TIMEOUT);
    m_requestParamsEdit->setPlainText(Constants::DEFAULT_REQUEST_PARAMETERS);
    m_autoExternalProcessCheck->setChecked(false);

    m_rendererFontSpin->setValue(Constants::DEFAULT_RENDERER_FONT_SIZE);
    m_sourceEditorFontSpin->setValue(Constants::DEFAULT_SOURCE_EDITOR_FONT_SIZE);

    // 【新增】恢复历史设置
    m_saveHistoryCheck->setChecked(Constants::DEFAULT_SAVE_HISTORY);
    m_historyLimitSpin->setValue(Constants::DEFAULT_HISTORY_LIMIT);

    m_silentModeCheck->setChecked(Constants::DEFAULT_SILENT_MODE_ENABLED);
    m_silentModeNotificationCombo->setCurrentIndex(
        m_silentModeNotificationCombo->findData(Constants::DEFAULT_SILENT_MODE_NOTIFICATION_TYPE));
    m_silentModeNotificationCombo->setEnabled(Constants::DEFAULT_SILENT_MODE_ENABLED);

    // 【新增】恢复新变量默认值
    m_textProcessorEdit->clear();
    m_textProcessorScEdit->clear();

    m_formulaProcessorEdit->clear();
    m_formulaProcessorScEdit->clear();

    m_tableProcessorEdit->clear();
    m_tableProcessorScEdit->clear();

    m_pureMathProcessorEdit->clear();
    m_pureMathProcessorScEdit->clear();

    m_autoStartServiceCheck->setChecked(Constants::DEFAULT_AUTO_START_SERVICE);

    m_floatingBallSizeSpin->setValue(Constants::DEFAULT_FLOATING_BALL_SIZE);
    m_floatingBallAutoHideTimeSpin->setValue(Constants::DEFAULT_FLOATING_BALL_AUTO_HIDE_TIME);
    m_floatingBallAlwaysVisibleCheck->setChecked(Constants::DEFAULT_FLOATING_BALL_ALWAYS_VISIBLE);

    m_formatterEnabledCheck->setChecked(Constants::DEFAULT_FORMATTER_ENABLED);
    m_formatterCommandEdit->setText(Constants::DEFAULT_FORMATTER_COMMAND);
    m_formatterOrderCombo->setCurrentIndex(
        m_formatterOrderCombo->findData(Constants::DEFAULT_FORMATTER_ORDER));
    m_formatterCommandEdit->setEnabled(false);
    m_formatterOrderCombo->setEnabled(false);
}

#endif // SETTINGSDIALOG_IMPL_H
