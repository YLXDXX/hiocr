// src/markdowncopybar.h
#ifndef MARKDOWNCOPYBAR_H
#define MARKDOWNCOPYBAR_H

#include <QWidget>
#include <QPushButton>
#include "copyprocessor.h" // 【新增】引入

class QPlainTextEdit;

class MarkdownCopyBar : public QWidget
{
    Q_OBJECT

public:
    // 枚举类型改用全局定义的 ContentType，这里可以删除本地的 enum

    explicit MarkdownCopyBar(QWidget* parent = nullptr);
    void setSourceEditor(QPlainTextEdit* editor);
    void executeCopy(const QString& text); // 保持 public 供外部调用

private slots:
    void onCopyOriginal();
    void onCopyInline();
    void onCopyDisplay();
    void onSourceTextChanged();
    // 删除 onExternalProcessFinished，逻辑移至 CopyProcessor

private:
    void setupUi();
    void updateButtonState(const QString& content);

    QPushButton* m_btnOriginal;
    QPushButton* m_btnInline;
    QPushButton* m_btnDisplay;

    QPlainTextEdit* m_sourceEdit = nullptr;
    ContentType m_currentType; // 使用全局枚举

    CopyProcessor* m_processor = nullptr; // 【新增】
};

#endif // MARKDOWNCOPYBAR_H
