#include "data/categoryrepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <utility>

namespace {

Category readCategory(const QSqlQuery &query)
{
    Category category;
    category.id = query.value(0).toInt();
    category.parentId = query.value(1).isNull() ? -1 : query.value(1).toInt();
    category.name = query.value(2).toString();
    category.type = categoryTypeFromString(query.value(3).toString());
    category.sortOrder = query.value(4).toInt();
    return category;
}

} // namespace

CategoryRepository::CategoryRepository(QSqlDatabase db)
    : m_db(std::move(db))
{
}

QList<Category> CategoryRepository::all(QString *error) const
{
    QList<Category> result;
    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral(
            "SELECT id, parent_id, name, type, sort_order FROM categories "
            "ORDER BY type, sort_order, name"))) {
        if (error) *error = query.lastError().text();
        return result;
    }
    while (query.next()) {
        result.append(readCategory(query));
    }
    return result;
}

QList<Category> CategoryRepository::withPaths(QString *error) const
{
    QList<Category> result;
    QSqlQuery query(m_db);
    const QString sql = QStringLiteral(
        "WITH RECURSIVE tree(id, parent_id, name, type, sort_order, depth, path) AS ("
        "  SELECT id, parent_id, name, type, sort_order, 0, name"
        "    FROM categories WHERE parent_id IS NULL"
        "  UNION ALL"
        "  SELECT c.id, c.parent_id, c.name, c.type, c.sort_order, t.depth + 1,"
        "         t.path || ' > ' || c.name"
        "    FROM categories c JOIN tree t ON c.parent_id = t.id)"
        " SELECT id, parent_id, name, type, sort_order, depth, path"
        "   FROM tree ORDER BY path");

    if (!query.exec(sql)) {
        if (error) *error = query.lastError().text();
        return result;
    }
    while (query.next()) {
        Category category;
        category.id = query.value(0).toInt();
        category.parentId = query.value(1).isNull() ? -1 : query.value(1).toInt();
        category.name = query.value(2).toString();
        category.type = categoryTypeFromString(query.value(3).toString());
        category.sortOrder = query.value(4).toInt();
        category.depth = query.value(5).toInt();
        category.path = query.value(6).toString();
        result.append(category);
    }
    return result;
}

Category CategoryRepository::findById(int id, bool *ok) const
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "SELECT id, parent_id, name, type, sort_order FROM categories WHERE id = ?"));
    query.addBindValue(id);
    if (query.exec() && query.next()) {
        if (ok) *ok = true;
        return readCategory(query);
    }
    if (ok) *ok = false;
    return Category();
}

bool CategoryRepository::add(int parentId, const QString &name, CategoryType type,
                             Category *out, QString *error)
{
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) {
        if (error) *error = QStringLiteral("El nombre no puede estar vacio.");
        return false;
    }

    if (parentId >= 0) {
        bool parentOk = false;
        const Category parent = findById(parentId, &parentOk);
        if (!parentOk) {
            if (error) *error = QStringLiteral("La categoria padre no existe.");
            return false;
        }
        type = parent.type;
    }

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "INSERT INTO categories (parent_id, name, type, sort_order) VALUES (?, ?, ?, 0)"));
    if (parentId < 0) {
        query.addBindValue(QVariant());
    } else {
        query.addBindValue(parentId);
    }
    query.addBindValue(trimmed);
    query.addBindValue(QString::fromUtf8(categoryTypeToString(type)));

    if (!query.exec()) {
        if (error) *error = query.lastError().text();
        return false;
    }
    if (out) {
        *out = findById(query.lastInsertId().toInt());
    }
    return true;
}

bool CategoryRepository::rename(int id, const QString &name, QString *error)
{
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) {
        if (error) *error = QStringLiteral("El nombre no puede estar vacio.");
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("UPDATE categories SET name = ? WHERE id = ?"));
    query.addBindValue(trimmed);
    query.addBindValue(id);
    if (!query.exec()) {
        if (error) *error = query.lastError().text();
        return false;
    }
    if (query.numRowsAffected() == 0) {
        if (error) *error = QStringLiteral("La categoria no existe.");
        return false;
    }
    return true;
}

bool CategoryRepository::move(int id, int newParentId, QString *error)
{
    if (id == newParentId) {
        if (error) *error = QStringLiteral("No se puede mover una categoria dentro de si misma.");
        return false;
    }

    bool movedOk = false;
    const Category moved = findById(id, &movedOk);
    if (!movedOk) {
        if (error) *error = QStringLiteral("La categoria no existe.");
        return false;
    }

    if (newParentId >= 0) {
        bool parentOk = false;
        const Category parent = findById(newParentId, &parentOk);
        if (!parentOk) {
            if (error) *error = QStringLiteral("La categoria destino no existe.");
            return false;
        }
        if (parent.type != moved.type) {
            if (error) *error = QStringLiteral("No se puede mover entre ingreso y gasto.");
            return false;
        }

        QSqlQuery descendants(m_db);
        descendants.prepare(QStringLiteral(
            "WITH RECURSIVE sub(id) AS ("
            "  SELECT id FROM categories WHERE id = ?"
            "  UNION ALL"
            "  SELECT c.id FROM categories c JOIN sub s ON c.parent_id = s.id)"
            " SELECT COUNT(*) FROM sub WHERE id = ?"));
        descendants.addBindValue(id);
        descendants.addBindValue(newParentId);
        if (!descendants.exec() || !descendants.next()) {
            if (error) *error = descendants.lastError().text();
            return false;
        }
        if (descendants.value(0).toInt() > 0) {
            if (error) *error = QStringLiteral("Movimiento invalido: crearia un ciclo.");
            return false;
        }
    }

    QSqlQuery update(m_db);
    update.prepare(QStringLiteral("UPDATE categories SET parent_id = ? WHERE id = ?"));
    if (newParentId < 0) {
        update.addBindValue(QVariant());
    } else {
        update.addBindValue(newParentId);
    }
    update.addBindValue(id);
    if (!update.exec()) {
        if (error) *error = update.lastError().text();
        return false;
    }
    return true;
}

bool CategoryRepository::remove(int id, QString *error)
{
    if (hasChildren(id)) {
        if (error) *error = QStringLiteral("La categoria tiene subcategorias.");
        return false;
    }
    if (isUsedByTransactions(id)) {
        if (error) *error = QStringLiteral("La categoria esta en uso por transacciones.");
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("DELETE FROM categories WHERE id = ?"));
    query.addBindValue(id);
    if (!query.exec()) {
        if (error) *error = query.lastError().text();
        return false;
    }
    if (query.numRowsAffected() == 0) {
        if (error) *error = QStringLiteral("La categoria no existe.");
        return false;
    }
    return true;
}

bool CategoryRepository::hasChildren(int id) const
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("SELECT COUNT(*) FROM categories WHERE parent_id = ?"));
    query.addBindValue(id);
    return query.exec() && query.next() && query.value(0).toInt() > 0;
}

bool CategoryRepository::isUsedByTransactions(int id) const
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("SELECT COUNT(*) FROM derived_transactions WHERE category_id = ?"));
    query.addBindValue(id);
    return query.exec() && query.next() && query.value(0).toInt() > 0;
}

QString CategoryRepository::pathOf(int id, bool *ok) const
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "WITH RECURSIVE up(id, parent_id, name, path) AS ("
        "  SELECT id, parent_id, name, name FROM categories WHERE id = ?"
        "  UNION ALL"
        "  SELECT c.id, c.parent_id, c.name, c.name || ' > ' || up.path"
        "    FROM categories c JOIN up ON c.id = up.parent_id)"
        " SELECT path FROM up WHERE parent_id IS NULL"));
    query.addBindValue(id);

    if (query.exec() && query.next()) {
        if (ok) *ok = true;
        return query.value(0).toString();
    }
    if (ok) *ok = false;
    return QString();
}
