# HiOCR 技术文档

该项目是一个基于 Qt6 框架开发的 ~~跨平台~~ 光学字符识别（OCR）桌面工具，可识别文字、公式和表格，特别针对 Linux 桌面 Wayland 环境进行了优化，在 KDE 桌面下的体验最好。

![Screenshot](Screenshot.png)

---

## 1. 项目概述

HiOCR 旨在提供高效的屏幕截图识别能力，支持将图片中的文字、数学公式、表格转换为 Markdown 格式。其核心特性包括：

*   **多模态识别**：支持文字、LaTeX 公式、表格三种预设识别模式。
*   **本地服务管理**：内置服务生命周期管理，支持自动启动本地 OCR 引擎、空闲自动关闭，以及进程组管理（防止僵尸进程）。
*   **高精度渲染**：基于 QWebEngineView 和 Temml 库提供高质量的 LaTeX 公式渲染，支持多种数学字体。
*   **智能复制**：提供三种复制策略（原样、行内公式、行间公式），支持复制前调用外部程序进行文本预处理。
*   **原生集成**：深度适配 Wayland（XDG Portal）和 X11，支持 KDE (KGlobalAccel) 和通用 Wayland (GlobalShortcuts Portal) 的全局快捷键。

---

## 2. 架构设计

项目采用分层架构，核心逻辑与 UI 解耦，通过 `AppController` 进行全局调度。

### 2.1 核心模块

*   **AppController**: 应用程序的核心控制器，负责协调各模块交互、生命周期管理、单实例通信及业务逻辑流转。
*   **MainWindow**: 主视图层，负责 UI 布局、用户交互事件的接收与转发。
*   **ScreenshotManager**: 截图管理器，封装 XDG Portal 和 X11 截图逻辑，输出 QImage。
*   **RecognitionManager**: 识别逻辑管理器，处理防抖、网络请求封装及错误重试逻辑。
*   **NetworkManager**: 网络层，负责 HTTP API 请求的构建与发送，兼容 OpenAI Chat Completion API 格式。
*   **ServiceManager**: 服务管理器，负责本地子进程的启动、停止、监控及空闲超时处理。
*   **SettingsManager**: 配置管理单例，封装 `QSettings`，提供类型安全的配置读写接口及信号通知。
*   **ShortcutHandler**: 快捷键处理器，根据运行环境动态选择 KGlobalAccel (KDE)、GlobalShortcuts Portal (Wayland) 或 Qt Local Shortcuts。

### 2.2 数据流

1.  **输入源**: 用户触发截图/粘贴/打开文件 -> `AppController` -> `ScreenshotManager`/`ImageViewWidget` -> 生成 `QImage`。
2.  **预处理**: `ImageProcessor` 将 `QImage` 转为 Base64。
3.  **识别请求**: `RecognitionManager` 组装 Prompt 和 Base64 -> `NetworkManager` 发送 POST 请求。
4.  **结果处理**: `NetworkManager` 接收 JSON -> 提取 Content -> `RecognitionManager` -> `MarkdownRenderer` (渲染) + `MarkdownSourceEditor` (源码)。
5.  **复制输出**: 用户点击复制 -> `MarkdownCopyBar` 判断内容类型 -> (可选) 调用外部程序 -> 写入剪贴板。

---

## 3. 编译指南

### 3.1 依赖项

*   **CMake** (>= 3.20)
*   **C++17** 编译器 (GCC/Clang)
*   **Qt6** 组件:
    *   `Core`, `Gui`, `Widgets`
    *   `Network`
    *   `WebEngineWidgets`, `WebChannel` (用于 Markdown 渲染)
    *   `DBus` (用于 Portal 通信)
*   **可选依赖**:
    *   `KF6GlobalAccel` (KDE Frameworks 6): 用于在 KDE 环境下注册原生全局快捷键。

### 3.2 编译步骤

```bash
# 1. 克隆仓库
git clone https://gitee.com/ylxdxx/hiocr
cd hiocr

# 2. 创建构建目录
mkdir build && cd build

# 3. 配置项目
cmake ..

# 4. 编译
make

# 5. 安装 (通常需要 root 权限)
sudo make install
```

*编译系统会自动检测 `KF6GlobalAccel`。如果找到，将自动启用 KDE 原生快捷键支持 (`-DHAVE_KF6`)。*

### 3.3 卸载

```bash
sudo make uninstall
```

---

## 4. 功能模块详解

### 4.1 图像输入系统

*   **截图 (`ScreenshotManager`)**:
    *   优先使用 `org.freedesktop.portal.Screenshot` (支持 Wayland 无 root 截图)。
    *   若 Portal 不可用，回退至 `QScreen::grabWindow` (X11)。
    *   截图后弹出 `ScreenshotAreaSelector`，支持高 DPI 缩放适配，通过计算物理像素与逻辑坐标的比例进行精确裁剪。
*   **其他输入**: 支持剪贴板粘贴 (`Ctrl+V`)、文件对话框打开、命令行参数传入。

### 4.2 识别引擎与服务交互

*   **网络请求 (`NetworkManager`)**:
    *   API 格式：OpenAI Chat Completion API (`/v1/chat/completions`)。
    *   支持 Base64 图片编码。
    *   支持从设置中加载高级请求参数（如 `temperature`, `max_tokens`），以 JSON 格式合并。
    *   **安全限制**：代码强制将 `stream` 设为 `false`，因为当前解析器不支持流式响应。
*   **服务管理 (`ServiceManager`)**:
    *   **启动**: 使用 `setpgid(0, 0)` 创建新进程组，确保能够通过 `kill(-pid)` 清理所有子进程。
    *   **空闲检测**: 维护一个 `QTimer`，每次识别活动后重置；超时后自动调用 `stopAllServices`。
    *   **故障恢复**: `AppController` 监听识别失败信号，若为连接错误，会自动检查配置并尝试启动本地服务进行重试。

### 4.3 Markdown 渲染

*   使用 `QWebEngineView` 加载本地 HTML 模板 (`qrc:/resources/markdown_renderer.html`)。
*   通过 `QWebChannel` 桥接 C++ 对象 (`MarkdownBridge`) 到 JS 环境。
*   **Temml 集成**: 支持实时渲染 LaTeX 数学公式。
*   **字体支持**: 内置 Latin Modern, Asana, STIX Two 等多种数学字体，通过动态加载 CSS 实现切换。

### 4.4 智能复制与外部处理

*   **内容检测**: 自动识别结果是否为 "单公式" (`$$...$$` 或 `$...$`) 或 "混合内容"。
*   **格式化策略**:
    *   `Original`: 保持源码或剥离公式包裹符号。
    *   `Inline`: 强制转为 `$...$`。
    *   `Display`: 根据 `displayMathEnvironment` 设置包裹公式（支持 `$$`, `equation`, `align` 等）。
*   **外部管道**: 复制前可执行用户配置的 Shell 命令，将文本通过 stdin 传入，stdout 传出，实现自定义预处理。

---

## 5. 配置系统

配置文件存储于 Qt 标准配置路径（通常为 `~/.config/hiocr/hiocr.conf`），由 `SettingsManager` 管理。

### 5.1 服务配置

支持配置多个识别服务配置文件，通过 JSON 序列化存储在 `services/list` 键下。

每个 `ServiceProfile` 包含：
*   `id`: 唯一标识
*   `name`: 显示名称
*   `start_command`: 启动命令（如 `llama-server -m model.gguf --port 8080`）
*   `server_url`: API 端点
*   `text_prompt` / `formula_prompt` / `table_prompt`: 特定于该服务的提示词

**切换模式**:
*   `SingleService`: 切换服务时自动停止其他服务（省资源）。
*   `ParallelServices`: 允许多个服务并行运行。

### 5.2 快捷键配置

*   支持配置：截图、文字识别、公式识别、表格识别、外部处理。
*   若键值为空，则取消快捷键绑定。
*   优先级：KGlobalAccel > Wayland Portal > Local。

### 5.3 高级参数

用户可在设置界面直接编辑 JSON 格式的请求参数，例如修改模型温度：

```json
{
    "temperature": 0.1,
    "max_tokens": 8192,
    "seed": -1
}
```

---

## 6. 命令行接口

程序支持命令行操作，方便脚本集成。

*   `hiocr`: 启动 GUI（若已运行则激活窗口）。
*   `hiocr -i <image_path>`: 启动并加载指定图片。
*   `hiocr -i <image_path> -r "<result_text>"`: 启动加载指定图片并直接显示结果文本（跳过识别，用于外部脚本传入结果）。
*   `hiocr --verbose`: 开启详细调试日志（显示 qDebug 输出）。

---

## 7. 开发者指南

### 7.1 单实例运行机制

通过 `QLocalServer` 实现。首次启动创建 Socket 服务端，后续启动尝试连接该 Socket 并发送 JSON 消息。消息格式：
```json
{
    "image": "absolute_path_to_image",
    "result": "optional_text_result"
}
```
主进程收到消息后调用 `handleCommandLineArguments` 处理。

### 7.2 DPI 适配逻辑

在 `ImageViewWidget` 和 `ScreenshotAreaSelector` 中，均监听 `QEvent::ScreenChangeInternal` 和 `QEvent::DevicePixelRatioChange` 事件。

*   **显示图片**: 使用 `screen->devicePixelRatioF()` 计算缩放因子，保证“原始大小”模式下一个图片像素对应一个物理像素。
*   **区域选择**: 截图返回的是物理像素 QImage，而窗口坐标是逻辑坐标，选区时通过 `scaleX/Y` 进行坐标映射。

---

## 8. 默认配置值

定义于 `src/constants.h`，主要包括：

*   **提示词**: 预设 Text, Formula, Table 识别提示词。
*   **服务器**: `http://localhost:8080/v1/chat/completions`。
*   **快捷键**: Text `Alt+M`, Formula `Ctrl+Alt+M`。
*   **服务**: 预置两个默认服务配置（本地 OCR 和远程 API 示例）。
*   **字体大小**: 渲染器与编辑器默认 16pt。

---

## 9. 使用示例

1.  **基本使用**:
    *   运行 `hiocr`。
    *   点击“截图”或使用快捷键（默认 `Alt+M` 进行文字识别）。
    *   框选屏幕区域。
    *   自动识别后，右侧显示 Markdown 源码，左侧渲染预览。
    *   点击下方复制按钮（内容/行内/行间）。

2.  **本地模型服务集成**:
    *   准备本地 LLM 服务
    *   在“设置” -> “识别服务管理”中添加新服务。
    *   填入启动命令和接口 URL。
    *   点击菜单栏“识别服务” -> “启动”。
    *   进行截图识别。

3.  **外部处理脚本**:
    *   编写 Bash 或 Python 脚本从 stdin 读取文本，处理后输出到 stdout。
    *   在设置中填入命令，如  `bash /path/to/script.sh`。
    *   勾选“复制前自动调用外部程序”。
    *   识别完成后点击复制，文本将自动经过脚本处理。
