#include "promptbar.h"
#include "settingsmanager.h"

#include <QHBoxLayout>
#include <QLabel>

PromptBar::PromptBar(QWidget* parent) : QWidget(parent)
{
    setupUi();
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    // 初始化默认提示词
    m_currentTextPrompt = SettingsManager::instance()->textPrompt();
    m_currentFormulaPrompt = SettingsManager::instance()->formulaPrompt();
    m_currentTablePrompt = SettingsManager::instance()->tablePrompt();
}

void PromptBar::setupUi()
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    layout->addWidget(new QLabel("提示词:"));

    m_promptEdit = new QLineEdit(this);
    m_promptEdit->setPlaceholderText("输入提示词...");
    layout->addWidget(m_promptEdit);

    m_textButton = new QPushButton("文字", this);
    m_formulaButton = new QPushButton("公式", this);
    m_tableButton = new QPushButton("表格", this);

    connect(m_textButton, &QPushButton::clicked, this, &PromptBar::onTextButton);
    connect(m_formulaButton, &QPushButton::clicked, this, &PromptBar::onFormulaButton);
    connect(m_tableButton, &QPushButton::clicked, this, &PromptBar::onTableButton);

    layout->addWidget(m_textButton);
    layout->addWidget(m_formulaButton);
    layout->addWidget(m_tableButton);

    m_recognizeButton = new QPushButton("识别", this);
    connect(m_recognizeButton, &QPushButton::clicked, this, &PromptBar::recognizeRequested);
    layout->addWidget(m_recognizeButton);
}

QString PromptBar::prompt() const
{
    return m_promptEdit->text();
}

void PromptBar::setPrompt(const QString& prompt)
{
    if (m_promptEdit->text() != prompt) {
        m_promptEdit->setText(prompt);
        emit promptChanged(prompt);
    }
}

void PromptBar::setButtonsBusy(bool busy)
{
    m_textButton->setEnabled(!busy);
    m_formulaButton->setEnabled(!busy);
    m_tableButton->setEnabled(!busy);
    m_recognizeButton->setEnabled(!busy);

    if (busy) {
        QString busyStyle = "background-color: #e0e0e0; color: #a0a0a0;";
        m_textButton->setStyleSheet(busyStyle);
        m_formulaButton->setStyleSheet(busyStyle);
        m_tableButton->setStyleSheet(busyStyle);
        m_recognizeButton->setStyleSheet(busyStyle);
    } else {
        m_textButton->setStyleSheet("");
        m_formulaButton->setStyleSheet("");
        m_tableButton->setStyleSheet("");
        m_recognizeButton->setStyleSheet("");
    }
}

// 【新增】实现
void PromptBar::setCurrentPrompts(const QString& text, const QString& formula, const QString& table)
{
    m_currentTextPrompt = text;
    m_currentFormulaPrompt = formula;
    m_currentTablePrompt = table;
}

void PromptBar::onTextButton()
{
    setPrompt(m_currentTextPrompt);
    emit autoRecognizeRequested(m_currentTextPrompt);
}

void PromptBar::onFormulaButton()
{
    setPrompt(m_currentFormulaPrompt);
    emit autoRecognizeRequested(m_currentFormulaPrompt);
}

void PromptBar::onTableButton()
{
    setPrompt(m_currentTablePrompt);
    emit autoRecognizeRequested(m_currentTablePrompt);
}
