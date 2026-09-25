#include "data/database.h"

#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>
#include <QVariant>

Database::Database() = default;

Database::~Database()
{
    close();
}

QString Database::defaultPath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return dir + QStringLiteral("/transacciones.db");
}

bool Database::open(const QString &path, QString *error)
{
    close();

    if (!QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE"))) {
        if (error) *error = QStringLiteral("El driver QSQLITE no esta disponible.");
        return false;
    }

    m_connectionName = QStringLiteral("gestion_%1")
                           .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));

    if (path != QStringLiteral(":memory:")) {
        const QFileInfo info(path);
        QDir().mkpath(info.absolutePath());
    }

    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
        db.setDatabaseName(path);
        if (!db.open()) {
            if (error) *error = db.lastError().text();
            db = QSqlDatabase();
            QSqlDatabase::removeDatabase(m_connectionName);
            m_connectionName.clear();
            return false;
        }

        QSqlQuery pragma(db);
        pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
    }

    if (!createSchema(error) || !seedCategories(error)) {
        close();
        return false;
    }
    return true;
}

bool Database::openDefault(QString *error)
{
    return open(defaultPath(), error);
}

void Database::close()
{
    if (m_connectionName.isEmpty()) {
        return;
    }

    if (QSqlDatabase::contains(m_connectionName)) {
        {
            QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
            if (db.isOpen()) {
                db.close();
            }
        }
        QSqlDatabase::removeDatabase(m_connectionName);
    }
    m_connectionName.clear();
}

bool Database::isOpen() const
{
    return !m_connectionName.isEmpty()
        && QSqlDatabase::contains(m_connectionName)
        && QSqlDatabase::database(m_connectionName, false).isOpen();
}

QSqlDatabase Database::connection() const
{
    return QSqlDatabase::database(m_connectionName, false);
}

bool Database::createSchema(QString *error)
{
    const QSqlDatabase db = connection();
    const QStringList statements = {
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS categories ("
            " id INTEGER PRIMARY KEY AUTOINCREMENT,"
            " parent_id INTEGER REFERENCES categories(id) ON DELETE RESTRICT,"
            " name TEXT NOT NULL,"
            " type TEXT NOT NULL CHECK (type IN ('ingreso','gasto')),"
            " sort_order INTEGER NOT NULL DEFAULT 0)"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS transactions ("
            " id INTEGER PRIMARY KEY AUTOINCREMENT,"
            " fecha TEXT NOT NULL,"
            " concepto TEXT NOT NULL,"
            " category_id INTEGER NOT NULL REFERENCES categories(id) ON DELETE RESTRICT,"
            " importe INTEGER NOT NULL,"
            " cuenta TEXT,"
            " notas TEXT,"
            " creado_en TEXT NOT NULL)"),
        QStringLiteral(
            "CREATE INDEX IF NOT EXISTS idx_transactions_category ON transactions(category_id)"),
        QStringLiteral(
            "CREATE INDEX IF NOT EXISTS idx_transactions_fecha ON transactions(fecha)"),
    };

    for (const QString &sql : statements) {
        QSqlQuery query(db);
        if (!query.exec(sql)) {
            if (error) *error = query.lastError().text();
            return false;
        }
    }
    return true;
}

bool Database::seedCategories(QString *error)
{
    const QSqlDatabase db = connection();

    QSqlQuery countQuery(db);
    if (!countQuery.exec(QStringLiteral("SELECT COUNT(*) FROM categories")) || !countQuery.next()) {
        if (error) *error = countQuery.lastError().text();
        return false;
    }
    if (countQuery.value(0).toInt() > 0) {
        return true;
    }

    struct SeedRow
    {
        const char *name;
        const char *parent;
        const char *type;
        int order;
    };

    const SeedRow seeds[] = {
        {"Gasto",         nullptr,     "gasto",   0},
        {"Ingreso",       nullptr,     "ingreso", 1},
        {"Comida",        "Gasto",     "gasto",   0},
        {"Restaurantes",  "Comida",    "gasto",   0},
        {"Supermercado",  "Comida",    "gasto",   1},
        {"Casa",          "Gasto",     "gasto",   1},
        {"Alquiler",      "Casa",      "gasto",   0},
        {"Suministros",   "Casa",      "gasto",   1},
        {"Transporte",    "Gasto",     "gasto",   2},
        {"Ocio",          "Gasto",     "gasto",   3},
        {"Salud",         "Gasto",     "gasto",   4},
        {"Salario",       "Ingreso",   "ingreso", 0},
        {"Fijo",          "Salario",   "ingreso", 0},
        {"Variable",      "Salario",   "ingreso", 1},
        {"Inversion",     "Ingreso",   "ingreso", 1},
        {"Otros",         "Ingreso",   "ingreso", 2},
    };

    QHash<QString, int> ids;
    for (const SeedRow &seed : seeds) {
        int parentId = -1;
        if (seed.parent != nullptr) {
            parentId = ids.value(QString::fromUtf8(seed.parent), -1);
            if (parentId < 0) {
                if (error) *error = QStringLiteral("Categoria padre no encontrada en el seed.");
                return false;
            }
        }

        QSqlQuery insert(db);
        insert.prepare(QStringLiteral(
            "INSERT INTO categories (parent_id, name, type, sort_order) VALUES (?, ?, ?, ?)"));
        if (parentId < 0) {
            insert.addBindValue(QVariant());
        } else {
            insert.addBindValue(parentId);
        }
        insert.addBindValue(QString::fromUtf8(seed.name));
        insert.addBindValue(QString::fromUtf8(seed.type));
        insert.addBindValue(seed.order);

        if (!insert.exec()) {
            if (error) *error = insert.lastError().text();
            return false;
        }
        ids.insert(QString::fromUtf8(seed.name), insert.lastInsertId().toInt());
    }

    return true;
}
