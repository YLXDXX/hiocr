#include "markdownsourceeditor.h"
#include <QFont>
#include <QTextDocument>
#include <QTextOption>

MarkdownSourceEditor::MarkdownSourceEditor(QWidget* parent)
: QPlainTextEdit(parent)
{
    QFont font("Courier New", 16);
    setFont(font);
    QTextDocument* doc = document();
    QTextOption option = doc->defaultTextOption();
    option.setAlignment(Qt::AlignLeft);
    doc->setDefaultTextOption(option);
}
