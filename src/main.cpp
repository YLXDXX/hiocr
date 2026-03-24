#include <QApplication>
#include <QCommandLineParser>
#include "appcontroller.h" // 引入 Controller
#include "mainwindow.h"

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

    // 创建控制器
    AppController controller;
    controller.initialize(); // 初始化所有 Manager 和连接

    // 解析命令行参数
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
            // 可以调用控制器的方法触发识别，或者通过信号
            // 这里简单展示：直接显示窗口并加载图片
        }
    }

    // 默认显示主窗口 (或者根据需求隐藏到托盘)
    // controller.mainWindow()->show();

    return app.exec();
}
