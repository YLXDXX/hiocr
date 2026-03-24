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
    // 【优化完成】清理旧的快捷键对象，使用 deleteLater() 更安全
    if (m_scScreenshot) { m_scScreenshot->deleteLater(); m_scScreenshot = nullptr; }
    if (m_scText) { m_scText->deleteLater(); m_scText = nullptr; }
    if (m_scFormula) { m_scFormula->deleteLater(); m_scFormula = nullptr; }
    if (m_scTable) { m_scTable->deleteLater(); m_scTable = nullptr; }

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
    if (QGuiApplication::platformName() != "wayland") return;

    bool isKde = qgetenv("XDG_CURRENT_DESKTOP").toLower().contains("kde");

    #ifdef HAVE_KF6
    if (isKde) {
        KdeGlobalShortcut* ks = KdeGlobalShortcut::instance();
        // 先断开旧的连接防止重复触发
        disconnect(ks, nullptr, this, nullptr);

        connect(ks, &KdeGlobalShortcut::activated, this, [this](const QString& id) {
            if (id == "screenshot") emit screenshotRequested();
            else if (id == "text_recognize") emit textRecognizeRequested();
            else if (id == "formula_recognize") emit formulaRecognizeRequested();
            else if (id == "table_recognize") emit tableRecognizeRequested();
        });

            SettingsManager* s = SettingsManager::instance();
            ks->registerShortcut("screenshot", "Screenshot", s->screenshotShortcut());
            ks->registerShortcut("text_recognize", "Text", s->textRecognizeShortcut());
            ks->registerShortcut("formula_recognize", "Formula", s->formulaRecognizeShortcut());
            ks->registerShortcut("table_recognize", "Table", s->tableRecognizeShortcut());
            ks->startListening();
            return;
    }
    #endif

    // Portal (Non-KDE Wayland)
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
