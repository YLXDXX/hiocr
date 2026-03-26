#include "shortcuthandler.h"
#include "settingsmanager.h"
#include "globalshortcutmanager.h"
#include "settingsmanager.h"

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
    // 无论如何都更新本地快捷键，作为保底方案
    setupLocalShortcuts();
    // 尝试设置全局快捷键（KDE 或 Wayland Portal）
    setupGlobalShortcuts();
}

void ShortcutHandler::setupLocalShortcuts()
{
    // 清理旧对象 (包括新增的 m_scExternalProcess)
    auto cleanup = [](QShortcut*& ptr) {
        if (ptr) { delete ptr; ptr = nullptr; }
    };
    cleanup(m_scScreenshot);
    cleanup(m_scText);
    cleanup(m_scFormula);
    cleanup(m_scTable);
    cleanup(m_scExternalProcess); // 【新增】

    SettingsManager* s = SettingsManager::instance();

    // 【修改】通用的创建函数：如果 key 为空则不创建
    auto createLocal = [this](QString key, QShortcut*& ptr, auto slot) {
        if (key.isEmpty()) return; // 空字符串 -> 取消绑定 (不创建对象)

        ptr = new QShortcut(QKeySequence(key), this);
        ptr->setContext(Qt::ApplicationShortcut);
        connect(ptr, &QShortcut::activated, this, slot);
    };

    createLocal(s->screenshotShortcut(), m_scScreenshot, &ShortcutHandler::screenshotRequested);
    createLocal(s->textRecognizeShortcut(), m_scText, &ShortcutHandler::textRecognizeRequested);
    createLocal(s->formulaRecognizeShortcut(), m_scFormula, &ShortcutHandler::formulaRecognizeRequested);
    createLocal(s->tableRecognizeShortcut(), m_scTable, &ShortcutHandler::tableRecognizeRequested);

    // 【新增】注册外部程序快捷键
    createLocal(s->externalProcessShortcut(), m_scExternalProcess, &ShortcutHandler::externalProcessRequested);
}

void ShortcutHandler::setupGlobalShortcuts()
{
    bool isKde = qgetenv("XDG_CURRENT_DESKTOP").toLower().contains("kde");

    #ifdef HAVE_KF6
    if (isKde) {
        KdeGlobalShortcut* ks = KdeGlobalShortcut::instance();
        disconnect(ks, nullptr, this, nullptr);

        connect(ks, &KdeGlobalShortcut::activated, this, [this](const QString& id) {
            if (id == "screenshot") emit screenshotRequested();
            else if (id == "text_recognize") emit textRecognizeRequested();
            else if (id == "formula_recognize") emit formulaRecognizeRequested();
            else if (id == "table_recognize") emit tableRecognizeRequested();
            // 【新增】处理外部程序快捷键
            else if (id == "external_process") emit externalProcessRequested();
        });

            SettingsManager* s = SettingsManager::instance();
            ks->registerShortcut("screenshot", "Screenshot", s->screenshotShortcut());
            ks->registerShortcut("text_recognize", "Text", s->textRecognizeShortcut());
            ks->registerShortcut("formula_recognize", "Formula", s->formulaRecognizeShortcut());
            ks->registerShortcut("table_recognize", "Table", s->tableRecognizeShortcut());
            // 【新增】注册外部程序快捷键
            ks->registerShortcut("external_process", "External Process", s->externalProcessShortcut());

            ks->startListening();
            return;
    }
    #endif

    // Wayland Portal 部分
    if (QGuiApplication::platformName() == "wayland") {
        GlobalShortcutManager* gsm = GlobalShortcutManager::instance();
        disconnect(gsm, nullptr, this, nullptr);

        connect(gsm, &GlobalShortcutManager::activated, this, [this](const QString& id) {
            if (id == "screenshot") emit screenshotRequested();
            else if (id == "text_recognize") emit textRecognizeRequested();
            else if (id == "formula_recognize") emit formulaRecognizeRequested();
            else if (id == "table_recognize") emit tableRecognizeRequested();
            // 【新增】处理外部程序快捷键
            else if (id == "external_process") emit externalProcessRequested();
        });

            SettingsManager* s = SettingsManager::instance();
            gsm->registerShortcut("screenshot", "Screenshot", s->screenshotShortcut());
            gsm->registerShortcut("text_recognize", "Text", s->textRecognizeShortcut());
            gsm->registerShortcut("formula_recognize", "Formula", s->formulaRecognizeShortcut());
            gsm->registerShortcut("table_recognize", "Table", s->tableRecognizeShortcut());
            // 【新增】注册外部程序快捷键
            gsm->registerShortcut("external_process", "External Process", s->externalProcessShortcut());

            gsm->startListening();
    }
}
