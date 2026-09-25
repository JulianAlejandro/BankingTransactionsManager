#include <QtTest>

#include <QSqlQuery>
#include <QTemporaryDir>

#include "data/categoryrepository.h"
#include "data/database.h"

namespace {

int userVersion(const Database &database)
{
    QSqlQuery query(database.connection());
    if (query.exec(QStringLiteral("PRAGMA user_version")) && query.next()) {
        return query.value(0).toInt();
    }
    return -1;
}

} // namespace

class TestDatabase : public QObject
{
    Q_OBJECT

private slots:
    void openCreatesSchemaAndSeedsData();
    void openResetsOlderSchemaWithExistingCategories();
};

void TestDatabase::openCreatesSchemaAndSeedsData()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("transacciones.db"));

    Database database;
    QString error;
    QVERIFY2(database.open(path, &error), qPrintable(error));
    QCOMPARE(userVersion(database), 2);

    CategoryRepository categories(database.connection());
    QVERIFY(categories.all().size() > 0);
}

void TestDatabase::openResetsOlderSchemaWithExistingCategories()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("transacciones.db"));

    {
        Database database;
        QString error;
        QVERIFY2(database.open(path, &error), qPrintable(error));
        QCOMPARE(userVersion(database), 2);

        // Simula una base de datos con esquema antiguo: las tablas tienen datos
        // (categories se autorreferencia) pero la version almacenada es inferior.
        QSqlQuery downgrade(database.connection());
        QVERIFY(downgrade.exec(QStringLiteral("PRAGMA user_version = 0")));
    }

    Database reopened;
    QString error;
    QVERIFY2(reopened.open(path, &error), qPrintable(error));
    QCOMPARE(userVersion(reopened), 2);

    CategoryRepository categories(reopened.connection());
    QVERIFY(categories.all().size() > 0);
}

QTEST_GUILESS_MAIN(TestDatabase)
#include "test_database.moc"
