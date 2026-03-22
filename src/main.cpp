#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>
#include <QFileInfo>
#include <QIcon>
#include <QStyle>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    // 环境变量设置
    qputenv("QT_NO_XDG_DESKTOP_PORTAL", "1");
    qputenv("QT_NO_GLOBAL_STATUSTRAY", "1");
    qputenv("XDG_ACTIVATION_TOKEN", "hiocr");

    QApplication app(argc, argv);

    // 【关键】设置应用在窗口全部关闭时不退出，配合托盘使用
    app.setQuitOnLastWindowClosed(false);

    // 设置应用信息
    QGuiApplication::setDesktopFileName("hiocr");
    QCoreApplication::setOrganizationName("hiocr");
    QCoreApplication::setApplicationName("hiocr");
    QCoreApplication::setApplicationVersion("0.1");

    // 设置应用图标（用于任务栏和托盘）
    // 如果系统主题中有 hiocr 图标，则使用；否则使用默认图标
    QApplication::setWindowIcon(QIcon::fromTheme("hiocr", QApplication::style()->standardIcon(QStyle::SP_ComputerIcon)));

    // 解析命令行参数
    QCommandLineParser parser;
    parser.setApplicationDescription("hiocr - 文字识别");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption imageOption(
        QStringList() << "i" << "image",
                                   "加载指定的图片文件 <file>",
                                   "file"
    );
    parser.addOption(imageOption);

    parser.process(app);

    MainWindow w;

    if (parser.isSet(imageOption)) {
        QString imagePath = parser.value(imageOption);
        QFileInfo fileInfo(imagePath);
        if (fileInfo.exists() && fileInfo.isFile()) {
            w.loadImageFromFile(imagePath);
        } else {
            qWarning() << "图片文件不存在或无效:" << imagePath;
        }
    }

    w.show();

    return app.exec();
}
