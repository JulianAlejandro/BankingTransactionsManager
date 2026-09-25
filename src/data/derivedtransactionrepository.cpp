#include "data/derivedtransactionrepository.h"

#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <utility>

namespace {

DerivedTransaction readDerived(const QSqlQuery &query)
{
    DerivedTransaction derived;
    derived.id = query.value(0).toInt();
    derived.transactionId = query.value(1).toInt();
    derived.categoryId = query.value(2).toInt();
    derived.concept = query.value(3).toString();
    derived.amountCents = query.value(4).toLongLong();
    derived.notes = query.value(5).toString();
    return derived;
}

const char *kSelectColumns =
    "SELECT id, transaction_id, category_id, concepto, importe, notas FROM derived_transactions";

QVariant optionalText(const QString &value)
{
    return value.isEmpty() ? QVariant() : QVariant(value);
}

} // namespace

DerivedTransactionRepository::DerivedTransactionRepository(QSqlDatabase db)
    : m_db(std::move(db))
{
}

QList<DerivedTransaction> DerivedTransactionRepository::listByParent(int transactionId,
                                                                     QString *error) const
{
    QList<DerivedTransaction> result;
    QSqlQuery query(m_db);
    query.prepare(QString::fromLatin1(kSelectColumns)
                  + QStringLiteral(" WHERE transaction_id = ? ORDER BY id"));
    query.addBindValue(transactionId);
    if (!query.exec()) {
        if (error) *error = query.lastError().text();
        return result;
    }
    while (query.next()) {
        result.append(readDerived(query));
    }
    return result;
}

QHash<int, QList<DerivedTransaction>> DerivedTransactionRepository::allGrouped(QString *error) const
{
    QHash<int, QList<DerivedTransaction>> result;
    QSqlQuery query(m_db);
    if (!query.exec(QString::fromLatin1(kSelectColumns)
                    + QStringLiteral(" ORDER BY transaction_id, id"))) {
        if (error) *error = query.lastError().text();
        return result;
    }
    while (query.next()) {
        const DerivedTransaction derived = readDerived(query);
        result[derived.transactionId].append(derived);
    }
    return result;
}

bool DerivedTransactionRepository::add(DerivedTransaction &derived, QString *error)
{
    if (!derived.isValid(error)) {
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "INSERT INTO derived_transactions"
        " (transaction_id, category_id, concepto, importe, notas, creado_en)"
        " VALUES (?, ?, ?, ?, ?, ?)"));
    query.addBindValue(derived.transactionId);
    query.addBindValue(derived.categoryId);
    query.addBindValue(optionalText(derived.concept));
    query.addBindValue(derived.amountCents);
    query.addBindValue(optionalText(derived.notes));
    query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));

    if (!query.exec()) {
        if (error) *error = query.lastError().text();
        return false;
    }
    derived.id = query.lastInsertId().toInt();
    return true;
}

bool DerivedTransactionRepository::update(const DerivedTransaction &derived, QString *error)
{
    if (!derived.isValid(error)) {
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "UPDATE derived_transactions SET category_id = ?, concepto = ?, importe = ?, notas = ?"
        " WHERE id = ?"));
    query.addBindValue(derived.categoryId);
    query.addBindValue(optionalText(derived.concept));
    query.addBindValue(derived.amountCents);
    query.addBindValue(optionalText(derived.notes));
    query.addBindValue(derived.id);

    if (!query.exec()) {
        if (error) *error = query.lastError().text();
        return false;
    }
    if (query.numRowsAffected() == 0) {
        if (error) *error = QStringLiteral("La transaccion derivada no existe.");
        return false;
    }
    return true;
}

bool DerivedTransactionRepository::remove(int id, QString *error)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("DELETE FROM derived_transactions WHERE id = ?"));
    query.addBindValue(id);
    if (!query.exec()) {
        if (error) *error = query.lastError().text();
        return false;
    }
    if (query.numRowsAffected() == 0) {
        if (error) *error = QStringLiteral("La transaccion derivada no existe.");
        return false;
    }
    return true;
}

bool DerivedTransactionRepository::deleteByParent(int transactionId, QString *error)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("DELETE FROM derived_transactions WHERE transaction_id = ?"));
    query.addBindValue(transactionId);
    if (!query.exec()) {
        if (error) *error = query.lastError().text();
        return false;
    }
    return true;
}
