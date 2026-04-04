// src/markdowncopybar.h
#ifndef MARKDOWNCOPYBAR_H
#define MARKDOWNCOPYBAR_H

#include <QWidget>
#include <QPushButton>
#include "copyprocessor.h"

class QPlainTextEdit;

class MarkdownCopyBar : public QWidget
{
    Q_OBJECT

public:
    explicit MarkdownCopyBar(QWidget* parent = nullptr);
    void setSourceEditor(QPlainTextEdit* editor);

    // 【新增】设置当前内容的原始识别类型
    void setOriginalRecognizeType(ContentType type);

    void executeCopy(const QString& text);

private slots:
    void onCopyOriginal();
    void onCopyInline();
    void onCopyDisplay();
    void onSourceTextChanged();

private:
    void setupUi();
    void updateButtonState(const QString& content);

    QPushButton* m_btnOriginal;
    QPushButton* m_btnInline;
    QPushButton* m_btnDisplay;

    QPlainTextEdit* m_sourceEdit = nullptr;
    CopyProcessor* m_processor = nullptr;

    ContentType m_currentType;           // 当前内容的结构类型 (Formula, Mixed, Text)
    ContentType m_originalRecognizeType; // 【新增】原始识别类型 (Text, Formula, Table)
};

#endif // MARKDOWNCOPYBAR_H
