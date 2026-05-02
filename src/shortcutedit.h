#ifndef SHORTCUTEDIT_H
#define SHORTCUTEDIT_H

#include <QLineEdit>
#include <QKeyEvent>

class ShortcutEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit ShortcutEdit(QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent* event) override;
};

inline ShortcutEdit::ShortcutEdit(QWidget* parent) : QLineEdit(parent)
{
    setReadOnly(true);
    setPlaceholderText("按下快捷键...");
}

inline void ShortcutEdit::keyPressEvent(QKeyEvent* event)
{
    int key = event->key();
    Qt::KeyboardModifiers modifiers = event->modifiers();

    if (key == Qt::Key_Control || key == Qt::Key_Shift ||
        key == Qt::Key_Alt || key == Qt::Key_Meta) {
        return;
        }

        if (key == Qt::Key_Backspace || key == Qt::Key_Delete) {
            clear();
            event->accept();
            return;
        }

        QKeySequence seq(key | modifiers);

        setText(seq.toString(QKeySequence::PortableText));

        event->accept();
}

#endif // SHORTCUTEDIT_H
