#include "data/transactionrepository.h"

#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <utility>

namespace {

Transaction readTransaction(const QSqlQuery &query)
{
    Transaction transaction;
    transaction.id = query.value(0).toInt();
    transaction.date = QDate::fromString(query.value(1).toString(), Qt::ISODate);
    transaction.concept = query.value(2).toString();
    transaction.categoryId = query.value(3).toInt();
    transaction.amountCents = query.value(4).toLongLong();
    transaction.account = query.value(5).toString();
    transaction.notes = query.value(6).toString();
    return transaction;
}

QVariant optionalText(const QString &value)
{
    return value.isEmpty() ? QVariant() : QVariant(value);
}

} // namespace

TransactionRepository::TransactionRepository(QSqlDatabase db)
    : m_db(std::move(db))
{
}

bool TransactionRepository::add(Transaction &transaction, QString *error)
{
    if (!transaction.isValid(error)) {
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "INSERT INTO transactions (fecha, concepto, category_id, importe, cuenta, notas, creado_en)"
        " VALUES (?, ?, ?, ?, ?, ?, ?)"));
    query.addBindValue(transaction.date.toString(Qt::ISODate));
    query.addBindValue(transaction.concept.trimmed());
    query.addBindValue(transaction.categoryId);
    query.addBindValue(transaction.amountCents);
    query.addBindValue(optionalText(transaction.account));
    query.addBindValue(optionalText(transaction.notes));
    query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));

    if (!query.exec()) {
        if (error) *error = query.lastError().text();
        return false;
    }
    transaction.id = query.lastInsertId().toInt();
    return true;
}

bool TransactionRepository::update(const Transaction &transaction, QString *error)
{
    if (!transaction.isValid(error)) {
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "UPDATE transactions SET fecha = ?, concepto = ?, category_id = ?, importe = ?,"
        " cuenta = ?, notas = ? WHERE id = ?"));
    query.addBindValue(transaction.date.toString(Qt::ISODate));
    query.addBindValue(transaction.concept.trimmed());
    query.addBindValue(transaction.categoryId);
    query.addBindValue(transaction.amountCents);
    query.addBindValue(optionalText(transaction.account));
    query.addBindValue(optionalText(transaction.notes));
    query.addBindValue(transaction.id);

    if (!query.exec()) {
        if (error) *error = query.lastError().text();
        return false;
    }
    if (query.numRowsAffected() == 0) {
        if (error) *error = QStringLiteral("La transaccion no existe.");
        return false;
    }
    return true;
}

bool TransactionRepository::remove(int id, QString *error)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("DELETE FROM transactions WHERE id = ?"));
    query.addBindValue(id);
    if (!query.exec()) {
        if (error) *error = query.lastError().text();
        return false;
    }
    if (query.numRowsAffected() == 0) {
        if (error) *error = QStringLiteral("La transaccion no existe.");
        return false;
    }
    return true;
}

QList<Transaction> TransactionRepository::all(QString *error) const
{
    QList<Transaction> result;
    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral(
            "SELECT id, fecha, concepto, category_id, importe, cuenta, notas"
            " FROM transactions ORDER BY fecha DESC, id DESC"))) {
        if (error) *error = query.lastError().text();
        return result;
    }
    while (query.next()) {
        result.append(readTransaction(query));
    }
    return result;
}
