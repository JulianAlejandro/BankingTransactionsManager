#include <QApplication>
#include <QFile>
#include <QString>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName("TuOrganizacion");
    QApplication::setOrganizationDomain("tuorganizacion.com");
    QApplication::setApplicationName("QT_Proy");
    QApplication::setApplicationVersion("0.1.0");

    QFile styleFile(":/styles/app.qss");
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        app.setStyleSheet(QString::fromUtf8(styleFile.readAll()));
    }

    MainWindow window;
    window.show();

    return app.exec();
}
