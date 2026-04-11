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
    void setOriginalRecognizeType(ContentType type);

    // 【新增】设置当前服务名称，用于判断复制行为
    void setCurrentServiceName(const QString& name);

    void executeCopy(const QString& text);

private slots:
    void onCopyOriginal();
    void onCopyInline();
    void onCopyDisplay();
    void onSourceTextChanged();

private:
    void setupUi();
    void updateButtonState(const QString& content);

    // 【新增】判断当前服务是否为 Chandra 模式
    bool isChandraService() const;

    QPushButton* m_btnOriginal;
    QPushButton* m_btnInline;
    QPushButton* m_btnDisplay;

    QPlainTextEdit* m_sourceEdit = nullptr;
    CopyProcessor* m_processor = nullptr;

    ContentType m_currentType;
    ContentType m_originalRecognizeType;

    // 【新增】当前服务名称
    QString m_currentServiceName;
};

#endif // MARKDOWNCOPYBAR_H
