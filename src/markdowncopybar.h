#ifndef MARKDOWNCOPYBAR_H
#define MARKDOWNCOPYBAR_H

#include <QWidget>
#include <QPushButton>

class QPlainTextEdit; // 前向声明

class MarkdownCopyBar : public QWidget
{
    Q_OBJECT

public:
    enum ContentType {
        SingleFormula,  // 纯公式
        MixedContent    // 混合内容
    };

    explicit MarkdownCopyBar(QWidget* parent = nullptr);

    // 【新增】设置源编辑器，用于实时获取文本
    void setSourceEditor(QPlainTextEdit* editor);

private slots:
    void onCopyOriginal();
    void onCopyInline();
    void onCopyDisplay();
    // 【新增】当文本变化时，重新判断内容类型
    void onSourceTextChanged();

private:
    void setupUi();
    void updateButtonState(const QString& content);
    QString transformToDisplayEnv(const QString& content);

    QPushButton* m_btnOriginal;
    QPushButton* m_btnInline;
    QPushButton* m_btnDisplay;

    QPlainTextEdit* m_sourceEdit = nullptr; // 持有编辑器指针
    ContentType m_currentType;
};

#endif // MARKDOWNCOPYBAR_H
