#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QString>

namespace Constants {
    // --- 提示词默认值（简短标签，完整提示词请参见 prompt.md）---
    const QString PROMPT_TEXT = "Text Recognition:";
    const QString PROMPT_FORMULA = "Formula Recognition:";
    const QString PROMPT_TABLE = "Table Recognition:";

    // --- 默认服务器地址 ---
    const QString DEFAULT_SERVER_URL = "http://localhost:8080/v1/chat/completions";
    // 全局默认 API 配置
    const QString DEFAULT_GLOBAL_API_KEY = "";
    const QString DEFAULT_GLOBAL_MODEL_NAME = ""; // 默认为空，让用户填写如 qwen-plus

    // --- 快捷键默认值 ---
    const QString SHORTCUT_SCREENSHOT = "";
    const QString SHORTCUT_TEXT = "Ctrl+Alt+M";
    const QString SHORTCUT_FORMULA = "";
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
    // 默认开启自动启动，提升用户体验
    const bool DEFAULT_AUTO_START_SERVICE = true;
    const QString DEFAULT_SERVICE_START_COMMAND = "";
    const int DEFAULT_SERVICE_IDLE_TIMEOUT = 10; // 默认10分钟

    // 默认请求参数 (JSON 格式)
    const QString DEFAULT_REQUEST_PARAMETERS = R"({
    "temperature": 0.5,
    "max_tokens": 8192,
    "cache_prompt": false,
    "enable_thinking": false,
    "stream":true
    })";


    // 默认图片预览模式 (0=整页, 1=同宽, 2=同高, 3=原大)
    const int DEFAULT_IMAGE_VIEW_MODE = 3;

    // 字体大小默认值
    const int DEFAULT_RENDERER_FONT_SIZE = 16;   // Markdown 渲染区默认字体大小
    const int DEFAULT_SOURCE_EDITOR_FONT_SIZE = 13; // Markdown 源码编辑区默认字体大小


    // --- 默认服务配置 ---
    // 提供两个默认服务配置示例
    const QString DEFAULT_SERVICE_1_NAME = "本地 OCR 服务";
    const QString DEFAULT_SERVICE_1_CMD = "llama-server -m /path/to/model.gguf --port 8080";
    const QString DEFAULT_SERVICE_1_URL = "http://localhost:8080/v1/chat/completions";

    const QString DEFAULT_SERVICE_2_NAME = "远程 API";
    const QString DEFAULT_SERVICE_2_CMD = ""; // 远程服务通常不需要启动命令
    const QString DEFAULT_SERVICE_2_URL = "https://api.example.com/v1/chat/completions";

    // 服务切换模式: 0=仅保留一个, 1=保留全部运行
    const int DEFAULT_SERVICE_SWITCH_MODE = 0;

    // 默认本地服务 ID 的配置键（仅用于初始化，实际存储在 settings 中）
    const QString DEFAULT_LOCAL_SERVICE_ID = "";


    // --- 复制前外部处理脚本配置 ---
    const QString DEFAULT_TEXT_PROCESSOR_CMD = "";
    const QString DEFAULT_FORMULA_PROCESSOR_CMD = "";
    const QString DEFAULT_TABLE_PROCESSOR_CMD = "";
    const QString DEFAULT_PURE_MATH_PROCESSOR_CMD = "";

    // --- 复制前处理快捷键 (默认为空) ---
    const QString SHORTCUT_TEXT_PROCESSOR = "";
    const QString SHORTCUT_FORMULA_PROCESSOR = "";
    const QString SHORTCUT_TABLE_PROCESSOR = "";
    const QString SHORTCUT_PURE_MATH_PROCESSOR = "";

    const QString SHORTCUT_ABORT = "Ctrl+.";

    // --- 脚本启用开关默认值 ---
    const bool DEFAULT_PROCESSOR_ENABLED = false;

    // 默认请求超时时间 (秒)
    const int DEFAULT_REQUEST_TIMEOUT = 30;

    // 历史记录设置
    const bool DEFAULT_SAVE_HISTORY = true;
    const int DEFAULT_HISTORY_LIMIT = 100;

    // --- 静默模式设置 ---
    const bool DEFAULT_SILENT_MODE_ENABLED = false;
    const QString DEFAULT_SILENT_MODE_NOTIFICATION_TYPE = "system_notification";
    // 可选值: "system_notification" (系统通知), "floating_ball" (悬浮小球)

    // --- 悬浮球设置 ---
    const int DEFAULT_FLOATING_BALL_SIZE = 48;
    const int DEFAULT_FLOATING_BALL_POS_X = -1; // -1表示未初始化，使用默认屏幕右下角
    const int DEFAULT_FLOATING_BALL_POS_Y = -1;
    const int DEFAULT_FLOATING_BALL_AUTO_HIDE_TIME = 5000; // 毫秒，0表示不自动隐藏
    const bool DEFAULT_FLOATING_BALL_ALWAYS_VISIBLE = false;

    // 历史记录分页设置
    const int HISTORY_PAGE_SIZE = 50;  // 每页显示的记录条数

    // --- LaTeX代码格式化工具设置 ---
    const bool DEFAULT_FORMATTER_ENABLED = false;
    const QString DEFAULT_FORMATTER_COMMAND = "";
    const int DEFAULT_FORMATTER_ORDER = 0; // 0=先格式化再外部处理, 1=先外部处理再格式化
}

#endif // CONSTANTS_H
