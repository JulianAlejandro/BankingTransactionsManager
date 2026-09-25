#ifndef CATEGORYREPOSITORY_H
#define CATEGORYREPOSITORY_H

#include <QList>
#include <QSqlDatabase>
#include <QString>

#include "model/category.h"

class CategoryRepository
{
public:
    explicit CategoryRepository(QSqlDatabase db = QSqlDatabase());

    QList<Category> all(QString *error = nullptr) const;
    QList<Category> withPaths(QString *error = nullptr) const;
    Category findById(int id, bool *ok = nullptr) const;

    // parentId < 0 crea una raiz (usa type). parentId >= 0 crea un hijo y hereda el type del padre.
    bool add(int parentId, const QString &name, CategoryType type,
             Category *out = nullptr, QString *error = nullptr);
    bool rename(int id, const QString &name, QString *error = nullptr);
    bool move(int id, int newParentId, QString *error = nullptr);
    bool remove(int id, QString *error = nullptr);

    bool hasChildren(int id) const;
    bool isUsedByTransactions(int id) const;
    QString pathOf(int id, bool *ok = nullptr) const;

private:
    QSqlDatabase m_db;
};

#endif // CATEGORYREPOSITORY_H
