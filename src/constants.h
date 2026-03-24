#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QString>

namespace Constants {
    // --- 提示词常量 ---
    const QString PROMPT_TEXT = "文字识别:";
    const QString PROMPT_FORMULA = "公式识别:";
    const QString PROMPT_TABLE = "表格识别:";

    // --- 默认服务器地址 ---
    const QString DEFAULT_SERVER_URL = "http://localhost:8080/v1/chat/completions";

    // --- 快捷键默认值 ---
    const QString SHORTCUT_SCREENSHOT = "Ctrl+Shift+S";
    const QString SHORTCUT_TEXT = "Ctrl+Shift+T";
    const QString SHORTCUT_FORMULA = "Ctrl+Shift+F";
    const QString SHORTCUT_TABLE = "Ctrl+Shift+B";

    // 【新增】默认行间公式环境
    // 可选值: "$$", "equation", "align" 等
    const QString DEFAULT_DISPLAY_MATH_ENV = "$$";

    const QString DEFAULT_EXTERNAL_PROCESSOR = ""; // 【新增】默认外部处理程序（空表示未配置）

    const bool DEFAULT_AUTO_COPY_RESULT = true;          // 默认开启自动复制
    const bool DEFAULT_AUTO_RECOGNIZE_SCREENSHOT = true; // 默认开启截图后自动识别
}

#endif // CONSTANTS_H
