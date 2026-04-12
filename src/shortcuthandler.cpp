#include "shortcuthandler.h"
#include "settingsmanager.h"
#include "globalshortcutmanager.h"

#ifdef HAVE_KF6
#include "kdeglobalshortcut.h"
#endif

#include <QShortcut>
#include <QGuiApplication>
#include <QDebug>

ShortcutHandler::ShortcutHandler(QObject *parent) : QObject(parent)
{
    applyShortcuts();
}

void ShortcutHandler::applyShortcuts()
{
    setupLocalShortcuts();
    setupGlobalShortcuts();
}

void ShortcutHandler::setupLocalShortcuts()
{
    // 辅助函数：清理旧快捷键
    auto cleanup = [](QShortcut*& ptr) {
        if (ptr) { delete ptr; ptr = nullptr; }
    };

    // 清理旧的识别快捷键
    cleanup(m_scScreenshot);
    cleanup(m_scText);
    cleanup(m_scFormula);
    cleanup(m_scTable);

    // 清理旧的处理脚本快捷键
    cleanup(m_scTextProcessor);
    cleanup(m_scFormulaProcessor);
    cleanup(m_scTableProcessor);
    cleanup(m_scPureMathProcessor);

    cleanup(m_scAbort);

    SettingsManager* s = SettingsManager::instance();

    // 辅助函数：创建本地快捷键
    auto createLocal = [this](QString key, QShortcut*& ptr, auto slot) {
        if (key.isEmpty()) return;
        ptr = new QShortcut(QKeySequence(key), this);
        ptr->setContext(Qt::ApplicationShortcut);
        connect(ptr, &QShortcut::activated, this, slot);
    };

    // 绑定识别快捷键
    createLocal(s->screenshotShortcut(), m_scScreenshot, &ShortcutHandler::screenshotRequested);
    createLocal(s->textRecognizeShortcut(), m_scText, &ShortcutHandler::textRecognizeRequested);
    createLocal(s->formulaRecognizeShortcut(), m_scFormula, &ShortcutHandler::formulaRecognizeRequested);
    createLocal(s->tableRecognizeShortcut(), m_scTable, &ShortcutHandler::tableRecognizeRequested);

    // 绑定处理脚本快捷键
    createLocal(s->textProcessorShortcut(), m_scTextProcessor, &ShortcutHandler::textProcessorRequested);
    createLocal(s->formulaProcessorShortcut(), m_scFormulaProcessor, &ShortcutHandler::formulaProcessorRequested);
    createLocal(s->tableProcessorShortcut(), m_scTableProcessor, &ShortcutHandler::tableProcessorRequested);
    createLocal(s->pureMathProcessorShortcut(), m_scPureMathProcessor, &ShortcutHandler::pureMathProcessorRequested);

    createLocal(s->abortShortcut(), m_scAbort, &ShortcutHandler::abortRequested);
}

void ShortcutHandler::setupGlobalShortcuts()
{
    bool isKde = qgetenv("XDG_CURRENT_DESKTOP").toLower().contains("kde");

    #ifdef HAVE_KF6
    if (isKde) {
        KdeGlobalShortcut* ks = KdeGlobalShortcut::instance();
        disconnect(ks, nullptr, this, nullptr);

        connect(ks, &KdeGlobalShortcut::activated, this, [this](const QString& id) {
            // 识别
            if (id == "screenshot") emit screenshotRequested();
            else if (id == "text_recognize") emit textRecognizeRequested();
            else if (id == "formula_recognize") emit formulaRecognizeRequested();
            else if (id == "table_recognize") emit tableRecognizeRequested();
            // 处理脚本
            else if (id == "text_processor") emit textProcessorRequested();
            else if (id == "formula_processor") emit formulaProcessorRequested();
            else if (id == "table_processor") emit tableProcessorRequested();
            else if (id == "pure_math_processor") emit pureMathProcessorRequested();
        });

            SettingsManager* s = SettingsManager::instance();
            // 注册识别
            ks->registerShortcut("screenshot", "Screenshot", s->screenshotShortcut());
            ks->registerShortcut("text_recognize", "Text Recognize", s->textRecognizeShortcut());
            ks->registerShortcut("formula_recognize", "Formula Recognize", s->formulaRecognizeShortcut());
            ks->registerShortcut("table_recognize", "Table Recognize", s->tableRecognizeShortcut());
            // 注册处理脚本
            ks->registerShortcut("text_processor", "Text Processor", s->textProcessorShortcut());
            ks->registerShortcut("formula_processor", "Formula Processor", s->formulaProcessorShortcut());
            ks->registerShortcut("table_processor", "Table Processor", s->tableProcessorShortcut());
            ks->registerShortcut("pure_math_processor", "Pure Math Processor", s->pureMathProcessorShortcut());

            ks->startListening();
            return;
    }
    #endif

    // Wayland Portal 部分
    if (QGuiApplication::platformName() == "wayland") {
        GlobalShortcutManager* gsm = GlobalShortcutManager::instance();
        disconnect(gsm, nullptr, this, nullptr);

        connect(gsm, &GlobalShortcutManager::activated, this, [this](const QString& id) {
            // 识别
            if (id == "screenshot") emit screenshotRequested();
            else if (id == "text_recognize") emit textRecognizeRequested();
            else if (id == "formula_recognize") emit formulaRecognizeRequested();
            else if (id == "table_recognize") emit tableRecognizeRequested();
            // 处理脚本
            else if (id == "text_processor") emit textProcessorRequested();
            else if (id == "formula_processor") emit formulaProcessorRequested();
            else if (id == "table_processor") emit tableProcessorRequested();
            else if (id == "pure_math_processor") emit pureMathProcessorRequested();
        });

            SettingsManager* s = SettingsManager::instance();
            gsm->registerShortcut("screenshot", "Screenshot", s->screenshotShortcut());
            gsm->registerShortcut("text_recognize", "Text Recognize", s->textRecognizeShortcut());
            gsm->registerShortcut("formula_recognize", "Formula Recognize", s->formulaRecognizeShortcut());
            gsm->registerShortcut("table_recognize", "Table Recognize", s->tableRecognizeShortcut());

            gsm->registerShortcut("text_processor", "Text Processor", s->textProcessorShortcut());
            gsm->registerShortcut("formula_processor", "Formula Processor", s->formulaProcessorShortcut());
            gsm->registerShortcut("table_processor", "Table Processor", s->tableProcessorShortcut());
            gsm->registerShortcut("pure_math_processor", "Pure Math Processor", s->pureMathProcessorShortcut());

            gsm->startListening();
    }
}
