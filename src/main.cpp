#include <QApplication>
#include <QCommandLineParser>
#include <QSocketNotifier>
#include "appcontroller.h"
#include "mainwindow.h"

// 【新增】Unix 信号处理相关头文件
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>

// 【新增】全局静态变量，用于信号通信
static int sigintFd[2];

// 【新增】信号处理函数
void intSignalHandler(int)
{
    // 向 socket 写入一个字节，触发 Qt 的事件循环
    char a = 1;
    ::write(sigintFd[0], &a, sizeof(a));
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

    // 【新增】设置 SIGINT (Ctrl+C) 信号捕获
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigintFd) == 0) {
        // 注册信号处理函数
        struct sigaction sa;
        sa.sa_handler = intSignalHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0; // 不使用 SA_RESTART
        sigaction(SIGINT, &sa, nullptr);

        // 创建 QSocketNotifier 监听 socket
        // 当信号发生时，socket 可读，Qt 事件循环会调用 lambda
        QSocketNotifier *notifier = new QSocketNotifier(sigintFd[1], QSocketNotifier::Read, &app);
        QObject::connect(notifier, &QSocketNotifier::activated, [&]() {
            // 读取并丢弃数据
            char tmp;
            ::read(sigintFd[1], &tmp, sizeof(tmp));

            qDebug() << "Caught Ctrl+C, quitting gracefully...";
            // 调用 quit 会触发 AppController 析构函数，从而停止服务
            QCoreApplication::quit();
        });
    }

    // 创建控制器
    AppController controller;
    controller.initialize();

    QCommandLineParser parser;
    parser.setApplicationDescription("hiocr - 文字识别");
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption imageOption("i", "加载指定的图片文件 <file>", "file");
    parser.addOption(imageOption);
    parser.process(app);

    if (parser.isSet(imageOption)) {
        QString path = parser.value(imageOption);
        QImage img(path);
        if (!img.isNull()) {
            controller.mainWindow()->setImage(img);
        }
    }

    return app.exec();
}
