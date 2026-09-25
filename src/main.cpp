#include <QApplication>
#include <QFile>
#include <QMessageBox>
#include <QString>

#include "appinfo.h"
#include "data/categoryrepository.h"
#include "data/database.h"
#include "data/transactionservice.h"
#include "ui/mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName(AppInfo::Organization);
    QApplication::setApplicationName(AppInfo::Name);
    QApplication::setApplicationVersion(AppInfo::Version);

    QFile styleFile(":/styles/app.qss");
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        app.setStyleSheet(QString::fromUtf8(styleFile.readAll()));
    }

    Database database;
    QString error;
    if (!database.openDefault(&error)) {
        QMessageBox::critical(nullptr, AppInfo::DisplayName,
                              QObject::tr("No se pudo abrir la base de datos:\n%1").arg(error));
        return 1;
    }

    TransactionService service(database.connection());
    CategoryRepository categoryRepository(database.connection());

    MainWindow window(&service, &categoryRepository);
    window.show();

    return app.exec();
}
