#ifndef SHORTCUTHANDLER_H
#define SHORTCUTHANDLER_H

#include <QObject>

class QShortcut;

class ShortcutHandler : public QObject
{
    Q_OBJECT
public:
    explicit ShortcutHandler(QObject *parent = nullptr);

    void applyShortcuts();

signals:
    // 识别相关
    void screenshotRequested();
    void textRecognizeRequested();
    void formulaRecognizeRequested();
    void tableRecognizeRequested();

    void abortRequested();  // 【新增】强制中断识别

    // 新增：分类型外部处理快捷键
    void textProcessorRequested();
    void formulaProcessorRequested();
    void tableProcessorRequested();
    void pureMathProcessorRequested();

private:
    void setupLocalShortcuts();
    void setupGlobalShortcuts();

    // 识别快捷键
    QShortcut* m_scScreenshot = nullptr;
    QShortcut* m_scText = nullptr;
    QShortcut* m_scFormula = nullptr;
    QShortcut* m_scTable = nullptr;

    QShortcut* m_scAbort = nullptr;  // 【新增】中断识别快捷键

    // 新增：处理脚本快捷键
    QShortcut* m_scTextProcessor = nullptr;
    QShortcut* m_scFormulaProcessor = nullptr;
    QShortcut* m_scTableProcessor = nullptr;
    QShortcut* m_scPureMathProcessor = nullptr;
};

#endif // SHORTCUTHANDLER_H
