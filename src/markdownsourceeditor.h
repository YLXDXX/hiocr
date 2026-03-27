#ifndef MARKDOWNSOURCEEDITOR_H
#define MARKDOWNSOURCEEDITOR_H

#include <QPlainTextEdit>

class MarkdownSourceEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit MarkdownSourceEditor(QWidget* parent = nullptr);

private slots:
    // 【新增】字体大小变化槽
    void onFontSizeChanged(int size);
};

#endif // MARKDOWNSOURCEEDITOR_H
