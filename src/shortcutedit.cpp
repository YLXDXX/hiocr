#include "shortcutedit.h"
#include <QKeyEvent>

ShortcutEdit::ShortcutEdit(QWidget* parent) : QLineEdit(parent)
{
    // 设置为只读，防止用户直接输入乱码，只能通过按键捕获
    setReadOnly(true);
    setPlaceholderText("按下快捷键...");
}

void ShortcutEdit::keyPressEvent(QKeyEvent* event)
{
    int key = event->key();
    Qt::KeyboardModifiers modifiers = event->modifiers();

    // 忽略单独的修饰键，因为它们不能构成完整的快捷键
    if (key == Qt::Key_Control || key == Qt::Key_Shift ||
        key == Qt::Key_Alt || key == Qt::Key_Meta) {
        return;
        }

        // 如果用户按下的是 Backspace 或 Delete，则清空当前设置（取消快捷键）
        if (key == Qt::Key_Backspace || key == Qt::Key_Delete) {
            clear();
            event->accept();
            return;
        }

        // 构建快捷键序列
        QKeySequence seq(key | modifiers);

        // 使用 PortableText 格式保存，这是跨平台存储快捷键的标准方式
        setText(seq.toString(QKeySequence::PortableText));

        event->accept();
}
