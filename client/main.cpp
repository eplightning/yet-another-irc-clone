#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyleSheet("QPushButton{background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 white, stop: 1 grey); "
                                "border-style: solid; "
                                "border-width: 5px; "
                                "border-radius: 10px;"
                                "min-width: 50px;"
                                "}");
    MainWindow w;
    w.show();

    return a.exec();
}
