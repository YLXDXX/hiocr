// src/markdowncopybar.h
#ifndef MARKDOWNCOPYBAR_H
#define MARKDOWNCOPYBAR_H

#include <QWidget>
#include <QPushButton>
#include <QProcess>

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

    // 【修改】将此函数改为 public，供 MainWindow 调用
    void executeCopy(const QString& text);

private slots:
    void onCopyOriginal();
    void onCopyInline();
    void onCopyDisplay();
    void onSourceTextChanged();
    void onExternalProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void setupUi();
    void updateButtonState(const QString& content);

    QPushButton* m_btnOriginal;
    QPushButton* m_btnInline;
    QPushButton* m_btnDisplay;

    QPlainTextEdit* m_sourceEdit = nullptr;
    ContentType m_currentType;
    QProcess* m_externalProcess = nullptr;
};

#endif // MARKDOWNCOPYBAR_H
