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

    // 设置所有操作按钮的忙碌状态（禁用并改变样式）
    void setButtonsBusy(bool busy);

signals:
    void promptChanged(const QString& prompt);
    void recognizeRequested();              // 点击“识别”按钮时发出
    void autoRecognizeRequested(const QString& prompt);  // 点击文字/公式/表格按钮时发出

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
};

#endif // PROMPTBAR_H
