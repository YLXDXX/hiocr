#include <QApplication>
#include <QCommandLineParser>
#include <QLocalSocket>
#include <QSocketNotifier>
#include <QJsonDocument>
#include <QJsonObject>
#include "appcontroller.h"
#include "mainwindow.h"

#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <iostream> // 【新增】用于手动输出帮助信息

// 单实例通信 Key
static const QString SINGLE_INSTANCE_KEY = "hiocr_single_instance_socket";
static int sigintFd[2];

void intSignalHandler(int)
{
    char a = 1;
    ::write(sigintFd[0], &a, sizeof(a));
}

// 尝试连接已运行的实例并发送参数
bool trySendToExistingInstance(const QString& imagePath, const QString& resultText)
{
    QLocalSocket socket;
    socket.connectToServer(SINGLE_INSTANCE_KEY, QIODevice::WriteOnly);

    if (socket.waitForConnected(500)) {
        qDebug() << "Sending arguments to running instance...";

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
    qputenv("QT_NO_XDG_DESKTOP_PORTAL", "1");
    qputenv("QT_NO_GLOBAL_STATUSTRAY", "1");
    qputenv("XDG_ACTIVATION_TOKEN", "hiocr");

    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    QGuiApplication::setDesktopFileName("hiocr");
    QCoreApplication::setOrganizationName("hiocr");
    QCoreApplication::setApplicationName("hiocr");
    QCoreApplication::setApplicationVersion("0.1");

    // 1. 设置命令行解析器
    QCommandLineParser parser;
    parser.setApplicationDescription("hiocr - 文字识别");
    // 注意：这里添加了标准的帮助和版本选项
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption imageOption("i", "加载指定的图片文件 <file>", "file");
    parser.addOption(imageOption);

    QCommandLineOption resultOption("r", "指定识别结果文本 <text>，将进入静态展示模式", "text");
    parser.addOption(resultOption);

    // 【关键修改】手动解析参数，避免 process() 内部调用 exit() 导致 QApplication 析构函数未执行
    if (!parser.parse(QCoreApplication::arguments())) {
        // 解析错误（如参数缺失），输出错误并返回
        qCritical() << parser.errorText();
        return 1;
    }

    // 手动处理帮助选项
    if (parser.isSet("help")) {
        std::cout << parser.helpText().toStdString() << std::endl;
        return 0; // 正常返回，触发 QApplication 析构
    }

    // 手动处理版本选项
    if (parser.isSet("version")) {
        std::cout << QCoreApplication::applicationName().toStdString()
        << " " << QCoreApplication::applicationVersion().toStdString() << std::endl;
        return 0; // 正常返回
    }

    QString imagePath = parser.value(imageOption);
    QString resultText = parser.value(resultOption);

    // 2. 尝试连接现有实例
    if (trySendToExistingInstance(imagePath, resultText)) {
        return 0;
    }

    // 3. 继续主程序启动逻辑
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigintFd) == 0) {
        struct sigaction sa;
        sa.sa_handler = intSignalHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGINT, &sa, nullptr);

        QSocketNotifier *notifier = new QSocketNotifier(sigintFd[1], QSocketNotifier::Read, &app);
        QObject::connect(notifier, &QSocketNotifier::activated, [&]() {
            char tmp;
            ::read(sigintFd[1], &tmp, sizeof(tmp));
            qDebug() << "Caught Ctrl+C, quitting gracefully...";
            QCoreApplication::quit();
        });
    }

    AppController controller;
    controller.initialize();

    if (!imagePath.isEmpty() || !resultText.isEmpty()) {
        controller.handleCommandLineArguments(imagePath, resultText);
    }

    return app.exec();
}
