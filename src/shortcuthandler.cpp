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
    // 无论如何都更新本地快捷键，作为保底方案
    setupLocalShortcuts();
    // 尝试设置全局快捷键（KDE 或 Wayland Portal）
    setupGlobalShortcuts();
}

void ShortcutHandler::setupLocalShortcuts()
{
    // 【保持之前的修复】使用 delete 立即销毁旧对象
    if (m_scScreenshot) { delete m_scScreenshot; m_scScreenshot = nullptr; }
    if (m_scText) { delete m_scText; m_scText = nullptr; }
    if (m_scFormula) { delete m_scFormula; m_scFormula = nullptr; }
    if (m_scTable) { delete m_scTable; m_scTable = nullptr; }

    SettingsManager* s = SettingsManager::instance();

    auto createLocal = [this](QString key, QShortcut*& ptr, auto slot) {
        if (key.isEmpty()) return;
        ptr = new QShortcut(QKeySequence(key), this);
        ptr->setContext(Qt::ApplicationShortcut);
        connect(ptr, &QShortcut::activated, this, slot);
    };

    createLocal(s->screenshotShortcut(), m_scScreenshot, &ShortcutHandler::screenshotRequested);
    createLocal(s->textRecognizeShortcut(), m_scText, &ShortcutHandler::textRecognizeRequested);
    createLocal(s->formulaRecognizeShortcut(), m_scFormula, &ShortcutHandler::formulaRecognizeRequested);
    createLocal(s->tableRecognizeShortcut(), m_scTable, &ShortcutHandler::tableRecognizeRequested);
}

void ShortcutHandler::setupGlobalShortcuts()
{
    // 【修改】移除原本阻止 X11 的判断 (platformName != "wayland")，
    // 因为在 KDE/X11 环境下也需要通过 KGlobalAccel 注册全局快捷键才能在后台生效。

    bool isKde = qgetenv("XDG_CURRENT_DESKTOP").toLower().contains("kde");

    #ifdef HAVE_KF6
    if (isKde) {
        KdeGlobalShortcut* ks = KdeGlobalShortcut::instance();
        // 断开旧连接防止重复（虽然 KdeGlobalShortcut 内部有去重，但外部断开更安全）
        disconnect(ks, nullptr, this, nullptr);

        connect(ks, &KdeGlobalShortcut::activated, this, [this](const QString& id) {
            if (id == "screenshot") emit screenshotRequested();
            else if (id == "text_recognize") emit textRecognizeRequested();
            else if (id == "formula_recognize") emit formulaRecognizeRequested();
            else if (id == "table_recognize") emit tableRecognizeRequested();
        });

            SettingsManager* s = SettingsManager::instance();
            // 调用修复后的注册函数，会强制更新 KDE 系统中的快捷键绑定
            ks->registerShortcut("screenshot", "Screenshot", s->screenshotShortcut());
            ks->registerShortcut("text_recognize", "Text", s->textRecognizeShortcut());
            ks->registerShortcut("formula_recognize", "Formula", s->formulaRecognizeShortcut());
            ks->registerShortcut("table_recognize", "Table", s->tableRecognizeShortcut());
            ks->startListening();
            return;
    }
    #endif

    // 仅在 Wayland 非 KDE 环境下使用 Portal
    if (QGuiApplication::platformName() == "wayland") {
        GlobalShortcutManager* gsm = GlobalShortcutManager::instance();
        disconnect(gsm, nullptr, this, nullptr);

        connect(gsm, &GlobalShortcutManager::activated, this, [this](const QString& id) {
            if (id == "screenshot") emit screenshotRequested();
            else if (id == "text_recognize") emit textRecognizeRequested();
            else if (id == "formula_recognize") emit formulaRecognizeRequested();
            else if (id == "table_recognize") emit tableRecognizeRequested();
        });

            SettingsManager* s = SettingsManager::instance();
            gsm->registerShortcut("screenshot", "Screenshot", s->screenshotShortcut());
            gsm->registerShortcut("text_recognize", "Text", s->textRecognizeShortcut());
            gsm->registerShortcut("formula_recognize", "Formula", s->formulaRecognizeShortcut());
            gsm->registerShortcut("table_recognize", "Table", s->tableRecognizeShortcut());
            gsm->startListening();
    }
}
