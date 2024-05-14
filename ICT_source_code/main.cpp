#include "transmissionwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv); // 创建 QApplication 对象
    TransmissionWindow *w = new TransmissionWindow; // 创建 TransmissionWindow 对象
    QIcon icon(":/file/image/logo.png");  // 设置窗口图标
    w->setWindowIcon(icon);
    w->setWindowTitle("Infotainment Content Transmission System"); // 设置窗口标题
    w->show(); // 显示窗口
    return a.exec(); // 启动事件循环
}
