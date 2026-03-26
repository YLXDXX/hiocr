#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QString>

namespace Constants {
    // --- 提示词常量 ---
    const QString PROMPT_TEXT = "Text Recognition:";
    const QString PROMPT_FORMULA = "Formula Recognition:";
    const QString PROMPT_TABLE = "Table Recognition:";

    // --- 默认服务器地址 ---
    const QString DEFAULT_SERVER_URL = "http://localhost:8080/v1/chat/completions";

    // --- 快捷键默认值 ---
    const QString SHORTCUT_SCREENSHOT = "";
    const QString SHORTCUT_TEXT = "Alt+M";
    const QString SHORTCUT_FORMULA = "Ctrl+Alt+M";
    const QString SHORTCUT_TABLE = "";
    const QString SHORTCUT_EXTERNAL_PROCESS = "";

    // --- 显示设置 ---
    const QString DEFAULT_DISPLAY_MATH_ENV = "$$";

    // 默认数学字体 (对应资源文件名后缀)
    // 可选值: "Latin-Modern", "Asana", "STIX2", "Libertinus", "NotoSans", "Local"
    const QString DEFAULT_MATH_FONT = "Latin-Modern";

    const QString DEFAULT_EXTERNAL_PROCESSOR = "";
    const bool DEFAULT_AUTO_COPY_RESULT = true;
    const bool DEFAULT_AUTO_RECOGNIZE_SCREENSHOT = true;
    const bool DEFAULT_AUTO_EXTERNAL_PROCESS_BEFORE_COPY = false;

    // --- 服务管理设置 ---
    const bool DEFAULT_AUTO_START_SERVICE = false; // 默认不自动启动，避免干扰在线服务用户
    const QString DEFAULT_SERVICE_START_COMMAND = "";
    const int DEFAULT_SERVICE_IDLE_TIMEOUT = 10; // 默认10分钟

    // 默认请求参数 (JSON 格式)
    // 注意：stream 固定为 false，因为当前代码不支持流式解析，这里不配置 stream
    const QString DEFAULT_REQUEST_PARAMETERS = R"({
    "temperature": 0.1,
    "max_tokens": 8192,
    "cache_prompt": false,
    "seed": -1
    })";


    // 默认图片预览模式 (0=整页, 1=同宽, 2=同高, 3=原大)
    const int DEFAULT_IMAGE_VIEW_MODE = 3;
}

#endif // CONSTANTS_H
