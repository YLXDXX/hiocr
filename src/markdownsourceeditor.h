#ifndef MARKDOWNSOURCEEDITOR_H
#define MARKDOWNSOURCEEDITOR_H

#include <QPlainTextEdit>

class MarkdownSourceEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit MarkdownSourceEditor(QWidget* parent = nullptr);
};

#endif // MARKDOWNSOURCEEDITOR_H
