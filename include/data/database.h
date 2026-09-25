#ifndef DATABASE_H
#define DATABASE_H

#include <QSqlDatabase>
#include <QString>

class Database
{
public:
    Database();
    ~Database();

    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

    bool open(const QString &path, QString *error = nullptr);
    bool openDefault(QString *error = nullptr);
    void close();
    bool isOpen() const;

    QSqlDatabase connection() const;

    static QString defaultPath();

private:
    int schemaVersion() const;
    bool resetSchema(QString *error);
    bool createSchema(QString *error);
    bool seedCategories(QString *error);
    bool setSchemaVersion(int version, QString *error);

    QString m_connectionName;
};

#endif // DATABASE_H
