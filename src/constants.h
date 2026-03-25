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

    // --- 显示设置 ---
    const QString DEFAULT_DISPLAY_MATH_ENV = "$$";

    // 【新增】默认数学字体 (对应资源文件名后缀)
    // 可选值: "Latin-Modern", "Asana", "STIX2", "Libertinus", "NotoSans", "Local"
    const QString DEFAULT_MATH_FONT = "Latin-Modern";

    const QString DEFAULT_EXTERNAL_PROCESSOR = "";

    const bool DEFAULT_AUTO_COPY_RESULT = true;
    const bool DEFAULT_AUTO_RECOGNIZE_SCREENSHOT = true;
}

#endif // CONSTANTS_H
