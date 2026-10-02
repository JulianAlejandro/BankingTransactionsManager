#include "data/reportrepository.h"

#include <algorithm>
#include <functional>
#include <utility>

#include <QDate>
#include <QHash>
#include <QMap>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "data/categoryrepository.h"

namespace {

QDate bucketStart(const QDate &date, Granularity granularity)
{
    switch (granularity) {
    case Granularity::Day:
        return date;
    case Granularity::Week:
        return date.addDays(-(date.dayOfWeek() - 1)); // lunes
    case Granularity::Month:
        return QDate(date.year(), date.month(), 1);
    case Granularity::Quarter:
        return QDate(date.year(), ((date.month() - 1) / 3) * 3 + 1, 1);
    case Granularity::Year:
        return QDate(date.year(), 1, 1);
    }
    return date;
}

QDate nextBucket(const QDate &start, Granularity granularity)
{
    switch (granularity) {
    case Granularity::Day:
        return start.addDays(1);
    case Granularity::Week:
        return start.addDays(7);
    case Granularity::Month:
        return start.addMonths(1);
    case Granularity::Quarter:
        return start.addMonths(3);
    case Granularity::Year:
        return start.addYears(1);
    }
    return start.addDays(1);
}

bool validPeriod(const ReportPeriod &period, QString *error)
{
    if (!period.isValid()) {
        if (error) *error = QStringLiteral("El periodo no es valido.");
        return false;
    }
    return true;
}

bool hasOutput(const void *out, QString *error)
{
    if (out == nullptr) {
        if (error) *error = QStringLiteral("La salida no puede ser nula.");
        return false;
    }
    return true;
}

QString isoDate(const QDate &date)
{
    return date.toString(Qt::ISODate);
}

} // namespace

ReportRepository::ReportRepository(QSqlDatabase db)
    : m_db(std::move(db))
{
}

bool ReportRepository::totals(const ReportPeriod &period, PeriodTotals *out, QString *error) const
{
    if (!validPeriod(period, error) || !hasOutput(out, error)) {
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "SELECT"
        "  COALESCE(SUM(CASE WHEN c.type = 'ingreso' THEN d.importe ELSE 0 END), 0),"
        "  COALESCE(SUM(CASE WHEN c.type = 'gasto' THEN d.importe ELSE 0 END), 0),"
        "  COUNT(DISTINCT t.id)"
        " FROM transactions t"
        " JOIN derived_transactions d ON d.transaction_id = t.id"
        " JOIN categories c ON c.id = d.category_id"
        " WHERE t.fecha BETWEEN ? AND ?"));
    query.addBindValue(isoDate(period.from));
    query.addBindValue(isoDate(period.to));

    if (!query.exec() || !query.next()) {
        if (error) *error = query.lastError().text();
        return false;
    }

    out->incomeCents = query.value(0).toLongLong();
    out->expenseCents = query.value(1).toLongLong();
    out->netCents = out->incomeCents + out->expenseCents;
    out->count = query.value(2).toInt();
    return true;
}

bool ReportRepository::categoryBreakdown(const ReportPeriod &period, int rootId,
                                         QList<CategoryTotal> *out, QString *error) const
{
    if (!validPeriod(period, error) || !hasOutput(out, error)) {
        return false;
    }

    CategoryRepository categories(m_db);
    const QList<Category> tree = categories.withPaths(error);
    if (tree.isEmpty()) {
        if (error && error->isEmpty()) {
            *error = QStringLiteral("No hay categorias.");
        }
        return false;
    }

    QHash<int, QList<int>> children;
    bool rootExists = false;
    for (const Category &category : tree) {
        children[category.parentId].append(category.id);
        if (category.id == rootId) {
            rootExists = true;
        }
    }
    if (!rootExists) {
        if (error) *error = QStringLiteral("La categoria raiz no existe.");
        return false;
    }

    // Conjunto de nodos del subarbol (recorrido en profundidad).
    QSet<int> subtree;
    std::function<void(int)> collect = [&](int id) {
        subtree.insert(id);
        for (int child : children.value(id)) {
            collect(child);
        }
    };
    collect(rootId);

    // Importes directos del periodo, agrupados por categoria.
    QHash<int, qint64> directCents;
    QHash<int, int> directCount;
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "SELECT d.category_id, COALESCE(SUM(d.importe), 0), COUNT(*)"
        " FROM transactions t"
        " JOIN derived_transactions d ON d.transaction_id = t.id"
        " WHERE t.fecha BETWEEN ? AND ?"
        " GROUP BY d.category_id"));
    query.addBindValue(isoDate(period.from));
    query.addBindValue(isoDate(period.to));
    if (!query.exec()) {
        if (error) *error = query.lastError().text();
        return false;
    }
    while (query.next()) {
        const int id = query.value(0).toInt();
        directCents.insert(id, query.value(1).toLongLong());
        directCount.insert(id, query.value(2).toInt());
    }

    // Acumulado (rama): se enrolla de hojas a raiz procesando por profundidad.
    QList<Category> subtreeNodes;
    for (const Category &category : tree) {
        if (subtree.contains(category.id)) {
            subtreeNodes.append(category);
        }
    }
    std::sort(subtreeNodes.begin(), subtreeNodes.end(),
              [](const Category &a, const Category &b) { return a.depth > b.depth; });

    QHash<int, qint64> subtreeCents;
    QHash<int, int> subtreeCount;
    for (const Category &category : subtreeNodes) {
        qint64 cents = directCents.value(category.id, 0);
        int count = directCount.value(category.id, 0);
        for (int child : children.value(category.id)) {
            if (subtree.contains(child)) {
                cents += subtreeCents.value(child, 0);
                count += subtreeCount.value(child, 0);
            }
        }
        subtreeCents.insert(category.id, cents);
        subtreeCount.insert(category.id, count);
    }

    out->clear();
    for (const Category &category : tree) {
        if (!subtree.contains(category.id)) {
            continue;
        }
        CategoryTotal total;
        total.categoryId = category.id;
        total.path = category.path;
        total.type = category.type;
        total.directCents = directCents.value(category.id, 0);
        total.directCount = directCount.value(category.id, 0);
        total.subtreeCents = subtreeCents.value(category.id, 0);
        total.subtreeCount = subtreeCount.value(category.id, 0);
        out->append(total);
    }
    return true;
}

bool ReportRepository::timeSeries(const ReportPeriod &period, Granularity granularity,
                                  QList<TimeBucket> *out, QString *error) const
{
    if (!validPeriod(period, error) || !hasOutput(out, error)) {
        return false;
    }

    QMap<QDate, TimeBucket> buckets;
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "SELECT t.fecha, c.type, d.importe"
        " FROM transactions t"
        " JOIN derived_transactions d ON d.transaction_id = t.id"
        " JOIN categories c ON c.id = d.category_id"
        " WHERE t.fecha BETWEEN ? AND ?"));
    query.addBindValue(isoDate(period.from));
    query.addBindValue(isoDate(period.to));
    if (!query.exec()) {
        if (error) *error = query.lastError().text();
        return false;
    }

    while (query.next()) {
        const QDate date = QDate::fromString(query.value(0).toString(), Qt::ISODate);
        if (!date.isValid()) {
            continue;
        }
        const QDate start = bucketStart(date, granularity);
        TimeBucket &bucket = buckets[start];
        bucket.start = start;
        const qint64 amount = query.value(2).toLongLong();
        if (categoryTypeFromString(query.value(1).toString()) == CategoryType::Income) {
            bucket.incomeCents += amount;
        } else {
            bucket.expenseCents += amount;
        }
    }

    out->clear();
    qint64 cumulative = 0;
    QDate cursor = bucketStart(period.from, granularity);
    const QDate last = bucketStart(period.to, granularity);
    while (cursor <= last) {
        TimeBucket bucket = buckets.value(cursor);
        bucket.start = cursor;
        bucket.netCents = bucket.incomeCents + bucket.expenseCents;
        cumulative += bucket.netCents;
        bucket.cumulativeNetCents = cumulative;
        out->append(bucket);

        const QDate next = nextBucket(cursor, granularity);
        if (next <= cursor) {
            break;
        }
        cursor = next;
    }
    return true;
}

bool ReportRepository::movements(const ReportPeriod &period, int categoryId,
                                 bool includeDescendants, QList<MovementRow> *out,
                                 QString *error) const
{
    if (!validPeriod(period, error) || !hasOutput(out, error)) {
        return false;
    }

    CategoryRepository categories(m_db);
    const QList<Category> tree = categories.withPaths(error);
    if (tree.isEmpty()) {
        if (error && error->isEmpty()) {
            *error = QStringLiteral("No hay categorias.");
        }
        return false;
    }

    QHash<int, QString> pathById;
    QHash<int, QList<int>> children;
    bool rootExists = false;
    for (const Category &category : tree) {
        pathById.insert(category.id, category.path);
        children[category.parentId].append(category.id);
        if (category.id == categoryId) {
            rootExists = true;
        }
    }
    if (!rootExists) {
        if (error) *error = QStringLiteral("La categoria no existe.");
        return false;
    }

    QSet<int> targetIds;
    std::function<void(int)> collect = [&](int id) {
        targetIds.insert(id);
        if (includeDescendants) {
            for (int child : children.value(id)) {
                collect(child);
            }
        }
    };
    collect(categoryId);

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "SELECT t.id, d.id, d.category_id, t.fecha,"
        "       COALESCE(NULLIF(d.concepto, ''), t.concepto),"
        "       d.importe, COALESCE(t.cuenta, '')"
        " FROM transactions t"
        " JOIN derived_transactions d ON d.transaction_id = t.id"
        " WHERE t.fecha BETWEEN ? AND ?"
        " ORDER BY t.fecha, t.id, d.id"));
    query.addBindValue(isoDate(period.from));
    query.addBindValue(isoDate(period.to));
    if (!query.exec()) {
        if (error) *error = query.lastError().text();
        return false;
    }

    out->clear();
    while (query.next()) {
        const int id = query.value(2).toInt();
        if (!targetIds.contains(id)) {
            continue;
        }
        MovementRow row;
        row.transactionId = query.value(0).toInt();
        row.derivedId = query.value(1).toInt();
        row.categoryId = id;
        row.date = QDate::fromString(query.value(3).toString(), Qt::ISODate);
        row.concept = query.value(4).toString();
        row.amountCents = query.value(5).toLongLong();
        row.account = query.value(6).toString();
        row.categoryPath = pathById.value(id);
        out->append(row);
    }
    return true;
}
