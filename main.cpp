#include "mainwindow.h"
#include "Configure/softconfig.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 初始化配置文件
    SoftConfig::getInstance()->init();

    // 加载样式
    qApp->setStyleSheet("file:///:/Resourse/qss/style.qss");

    MainWindow w;
    w.show();

    return a.exec();
}
