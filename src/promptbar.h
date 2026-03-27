#ifndef PROMPTBAR_H
#define PROMPTBAR_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>

class PromptBar : public QWidget
{
    Q_OBJECT

public:
    explicit PromptBar(QWidget* parent = nullptr);

    QString prompt() const;
    void setPrompt(const QString& prompt);
    void setButtonsBusy(bool busy);

    // 【新增】设置当前服务的提示词
    void setCurrentPrompts(const QString& text, const QString& formula, const QString& table);

signals:
    void promptChanged(const QString& prompt);
    void recognizeRequested();
    void autoRecognizeRequested(const QString& prompt);

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

    // 【新增】存储当前服务的提示词
    QString m_currentTextPrompt;
    QString m_currentFormulaPrompt;
    QString m_currentTablePrompt;
};

#endif // PROMPTBAR_H
