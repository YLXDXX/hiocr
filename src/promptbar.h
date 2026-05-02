// src/promptbar.h
#ifndef PROMPTBAR_H
#define PROMPTBAR_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>
#include "copyprocessor.h"

#include "settingsmanager.h"

class PromptBar : public QWidget
{
    Q_OBJECT

public:
    explicit PromptBar(QWidget* parent = nullptr);

    QString prompt() const;
    void setPrompt(const QString& prompt);
    void setButtonsBusy(bool busy);
    void setCurrentPrompts(const QString& text, const QString& formula, const QString& table);

signals:
    void promptChanged(const QString& prompt);
    void recognizeRequested();
    void autoRecognizeRequested(const QString& prompt, ContentType type);

private slots:
    void onTextButton();
    void onFormulaButton();
    void onTableButton();

private:
    void setupUi();

    QLineEdit* m_promptEdit;
    QPushButton* m_recognizeButton;
    QPushButton* m_textButton;
    QPushButton* m_formulaButton;
    QPushButton* m_tableButton;

    QString m_currentTextPrompt;
    QString m_currentFormulaPrompt;
    QString m_currentTablePrompt;
};

// --- Implementation ---

inline PromptBar::PromptBar(QWidget* parent) : QWidget(parent)
{
    setupUi();
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    m_currentTextPrompt = SettingsManager::instance()->textPrompt();
    m_currentFormulaPrompt = SettingsManager::instance()->formulaPrompt();
    m_currentTablePrompt = SettingsManager::instance()->tablePrompt();
}

inline void PromptBar::setupUi()
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

inline QString PromptBar::prompt() const
{
    return m_promptEdit->text();
}

inline void PromptBar::setPrompt(const QString& prompt)
{
    if (m_promptEdit->text() != prompt) {
        m_promptEdit->setText(prompt);
        emit promptChanged(prompt);
    }
}

inline void PromptBar::setButtonsBusy(bool busy)
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

inline void PromptBar::setCurrentPrompts(const QString& text, const QString& formula, const QString& table)
{
    m_currentTextPrompt = text;
    m_currentFormulaPrompt = formula;
    m_currentTablePrompt = table;
}

inline void PromptBar::onTextButton()
{
    setPrompt(m_currentTextPrompt);
    emit autoRecognizeRequested(m_currentTextPrompt, ContentType::Text);
}

inline void PromptBar::onFormulaButton()
{
    setPrompt(m_currentFormulaPrompt);
    emit autoRecognizeRequested(m_currentFormulaPrompt, ContentType::Formula);
}

inline void PromptBar::onTableButton()
{
    setPrompt(m_currentTablePrompt);
    emit autoRecognizeRequested(m_currentTablePrompt, ContentType::Table);
}

#endif // PROMPTBAR_H
