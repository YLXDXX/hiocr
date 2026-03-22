#include "promptbar.h"
#include <QHBoxLayout>
#include <QLabel>

PromptBar::PromptBar(QWidget* parent) : QWidget(parent)
{
    setupUi();
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
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
    // 禁用/启用所有操作按钮（文字、公式、表格、识别）
    m_textButton->setEnabled(!busy);
    m_formulaButton->setEnabled(!busy);
    m_tableButton->setEnabled(!busy);
    m_recognizeButton->setEnabled(!busy);

    // 改变按钮背景色以提供视觉反馈（可根据需要调整颜色）
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

void PromptBar::onTextButton()
{
    QString newPrompt = "文字识别:";
    setPrompt(newPrompt);
    emit autoRecognizeRequested(newPrompt);
}

void PromptBar::onFormulaButton()
{
    QString newPrompt = "公式识别:";
    setPrompt(newPrompt);
    emit autoRecognizeRequested(newPrompt);
}

void PromptBar::onTableButton()
{
    QString newPrompt = "表格识别:";
    setPrompt(newPrompt);
    emit autoRecognizeRequested(newPrompt);
}
