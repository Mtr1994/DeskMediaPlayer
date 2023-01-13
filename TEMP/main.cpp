#include "mainwindow.h"
#include "Configure/softconfig.h"

#include <QApplication>
#include <QDir>

// test
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QDir::setCurrent(QApplication::applicationDirPath());

    // 初始化配置文件
    SoftConfig::getInstance()->init();

    if (argc >= 2)
    {
        SoftConfig::getInstance()->setValue("Media", "path", QString::fromLocal8Bit(argv[1]));
    }

    // 加载样式
    qApp->setStyleSheet("file:///:/Resource/qss/style.qss");

    MainWindow w;
    w.show();

    return a.exec();
}
