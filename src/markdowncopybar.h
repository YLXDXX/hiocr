#ifndef MARKDOWNCOPYBAR_H
#define MARKDOWNCOPYBAR_H

#include <QWidget>
#include <QPushButton>
#include <QProcess> // 【新增】

class QPlainTextEdit;

class MarkdownCopyBar : public QWidget
{
    Q_OBJECT

public:
    enum ContentType {
        SingleFormula,
        MixedContent
    };

    explicit MarkdownCopyBar(QWidget* parent = nullptr);
    void setSourceEditor(QPlainTextEdit* editor);

private slots:
    void onCopyOriginal();
    void onCopyInline();
    void onCopyDisplay();
    void onSourceTextChanged();

    // 【新增】外部程序处理结束槽
    void onExternalProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void setupUi();
    void updateButtonState(const QString& content);

    // 【新增】核心复制逻辑（支持外部处理）
    void executeCopy(const QString& text);

    QPushButton* m_btnOriginal;
    QPushButton* m_btnInline;
    QPushButton* m_btnDisplay;

    QPlainTextEdit* m_sourceEdit = nullptr;
    ContentType m_currentType;

    // 【新增】用于临时存储待复制内容，等待外部程序处理
    QProcess* m_externalProcess = nullptr;
};

#endif // MARKDOWNCOPYBAR_H
