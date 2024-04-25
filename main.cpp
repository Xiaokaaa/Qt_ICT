#include "transmissionwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    TransmissionWindow *w = new TransmissionWindow;
    QIcon icon(":/file/image/logo.png");
    w->setWindowTitle("Infotainment Content Transmission System");
    w->setWindowIcon(icon);
    w->show();
    return a.exec();
}
