#include <QApplication>
#include <QCommandLineParser>
#include <QLocalSocket>
#include <QSocketNotifier>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileInfo>
#include "appcontroller.h"
#include "mainwindow.h"
#include "singleapplication.h" // 【新增】
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <iostream>
#include <stdio.h> // 【新增】用于 fprintf

// 单实例通信 Key
static const QString SINGLE_INSTANCE_KEY = "hiocr_single_instance_socket";
static int sigintFd[2];

// 【新增】全局变量：控制是否输出调试信息
static bool g_verboseMode = false;

// 【新增】自定义消息处理函数
void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    // 如果不是详细模式，则屏蔽 Debug 和 Info 类型的日志
    if (!g_verboseMode) {
        if (type == QtDebugMsg || type == QtInfoMsg) {
            return;
        }
    }

    // 这里我们可以统一控制输出格式
    QByteArray localMsg = msg.toLocal8Bit();
    const char *file = context.file ? context.file : "";
    const char *function = context.function ? context.function : "";
    const char *typeStr = "";

    switch (type) {
        case QtDebugMsg: typeStr = "Debug"; break;
        case QtInfoMsg: typeStr = "Info"; break;
        case QtWarningMsg: typeStr = "Warning"; break;
        case QtCriticalMsg: typeStr = "Critical"; break;
        case QtFatalMsg: typeStr = "Fatal"; break;
    }

    // 如果开启了详细模式，输出包含文件名和行号的详细信息
    // 否则只输出简略信息
    if (g_verboseMode) {
        fprintf(stderr, "[%s] %s (%s:%u, %s)\n",
                typeStr, localMsg.constData(), file, context.line, function);
    } else {
        // 非详细模式下，Warning 和 Critical 仍然输出，但更简洁
        fprintf(stderr, "[%s] %s\n", typeStr, localMsg.constData());
    }
}

void intSignalHandler(int)
{
    char a = 1;
    ::write(sigintFd[0], &a, sizeof(a));
}

// 发送参数前，先处理路径格式
bool trySendToExistingInstance(QString imagePath, const QString& resultText)
{
    // 【关键修复】如果是相对路径，转换为绝对路径
    // 因为主实例的工作目录可能与当前进程不同
    if (!imagePath.isEmpty()) {
        QFileInfo fileInfo(imagePath);
        if (fileInfo.isRelative()) {
            imagePath = fileInfo.absoluteFilePath();
        }
    }

    QLocalSocket socket;
    socket.connectToServer(SINGLE_INSTANCE_KEY, QIODevice::WriteOnly);

    if (socket.waitForConnected(500)) {
        // 这里故意不使用 qDebug，因为此时如果消息处理器已安装但 verbose 未设置，这行日志可能会被吞掉
        // 不过作为客户端，通常日志是给用户看的，这里保持静默或直接打印即可
        QJsonObject obj;
        obj["image"] = imagePath;
        obj["result"] = resultText;
        QByteArray data = QJsonDocument(obj).toJson();

        socket.write(data);
        socket.waitForBytesWritten(1000);
        socket.disconnectFromServer();

        return true;
    }

    return false;
}


int main(int argc, char *argv[])
{
    // 【关键】在创建 QApplication 之前安装消息处理程序
    qInstallMessageHandler(customMessageHandler);

    qputenv("QT_NO_XDG_DESKTOP_PORTAL", "1");
    qputenv("QT_NO_GLOBAL_STATUSTRAY", "1");
    qputenv("XDG_ACTIVATION_TOKEN", "hiocr");

    // 【修改】使用新的构造函数签名 (argc, argv, key)
    SingleApplication app(argc, argv, SINGLE_INSTANCE_KEY);

    // 将实例保存到全局属性
    app.setProperty("singleApp", QVariant::fromValue(&app));

    app.setQuitOnLastWindowClosed(false);

    QGuiApplication::setDesktopFileName("hiocr");
    QCoreApplication::setOrganizationName("hiocr");
    QCoreApplication::setApplicationName("hiocr");
    QCoreApplication::setApplicationVersion("0.1");

    // 1. 设置命令行解析器
    QCommandLineParser parser;
    parser.setApplicationDescription("HiOCR - 一款好用的识别工具");
    parser.addHelpOption();
    parser.addVersionOption();

    // 【新增】添加 verbose 选项
    QCommandLineOption verboseOption("verbose", "Enable debug output (qDebug/qInfo)");
    parser.addOption(verboseOption);

    QCommandLineOption imageOption("i", "加载指定的图片文件 <file>", "file");
    parser.addOption(imageOption);

    QCommandLineOption resultOption("r", "指定识别结果文本 <text>，将进入静态展示模式", "text");
    parser.addOption(resultOption);

    // 手动解析参数
    if (!parser.parse(QCoreApplication::arguments())) {
        qCritical() << parser.errorText();
        return 1;
    }

    // 手动处理帮助选项
    if (parser.isSet("help")) {
        std::cout << parser.helpText().toStdString() << std::endl;
        return 0;
    }

    if (parser.isSet("version")) {
        std::cout << QCoreApplication::applicationName().toStdString()
        << " " << QCoreApplication::applicationVersion().toStdString() << std::endl;
        return 0;
    }

    // 【新增】检查是否开启了详细模式
    if (parser.isSet(verboseOption)) {
        g_verboseMode = true;
        qDebug() << "Verbose mode enabled.";
    }

    QString imagePath = parser.value(imageOption);
    QString resultText = parser.value(resultOption);

    // 2. 检查单实例
    if (app.isRunning()) {
        QJsonObject obj;
        obj["image"] = imagePath;
        obj["result"] = resultText;
        app.sendMessage(QJsonDocument(obj).toJson());
        return 0;
    }

    AppController controller;
    controller.initialize();

    if (!imagePath.isEmpty() || !resultText.isEmpty()) {
        controller.handleCommandLineArguments(imagePath, resultText);
    }

    return app.exec();
}
