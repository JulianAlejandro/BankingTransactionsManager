#include "data/transactionservice.h"

#include <QLocale>
#include <QObject>
#include <QSqlError>
#include <utility>

namespace {

QLocale amountLocale()
{
    return QLocale(QLocale::Spanish, QLocale::Spain);
}

QString formatCents(qint64 cents)
{
    return amountLocale().toCurrencyString(static_cast<double>(cents) / 100.0,
                                           QStringLiteral("\u20ac"));
}

} // namespace

TransactionService::TransactionService(QSqlDatabase db)
    : m_db(std::move(db))
    , m_transactions(m_db)
    , m_derived(m_db)
    , m_categories(m_db)
{
}

bool TransactionService::validateAggregate(const Transaction &transaction,
                                           const QList<DerivedTransaction> &derived,
                                           QString *error) const
{
    if (!transaction.isValid(error)) {
        return false;
    }
    if (derived.isEmpty()) {
        if (error) *error = QObject::tr("Debe existir al menos una transaccion derivada.");
        return false;
    }

    qint64 sum = 0;
    for (const DerivedTransaction &item : derived) {
        if (!item.isValid(error)) {
            return false;
        }

        bool categoryOk = false;
        const Category category = m_categories.findById(item.categoryId, &categoryOk);
        if (!categoryOk) {
            if (error) *error = QObject::tr("La categoria de una derivada no existe.");
            return false;
        }

        const bool positive = item.amountCents > 0;
        if (category.type == CategoryType::Income && !positive) {
            if (error) *error = QObject::tr("Una derivada de ingreso debe ser positiva.");
            return false;
        }
        if (category.type == CategoryType::Expense && positive) {
            if (error) *error = QObject::tr("Una derivada de gasto debe ser negativa.");
            return false;
        }
        sum += item.amountCents;
    }

    if (sum != transaction.amountCents) {
        if (error) {
            *error = QObject::tr("La suma de las derivadas (%1) no coincide con el importe (%2).")
                         .arg(formatCents(sum), formatCents(transaction.amountCents));
        }
        return false;
    }
    return true;
}

bool TransactionService::load(int id, Transaction *transaction,
                              QList<DerivedTransaction> *derived, QString *error) const
{
    bool ok = false;
    const Transaction found = m_transactions.findById(id, &ok);
    if (!ok) {
        if (error) *error = QObject::tr("La transaccion no existe.");
        return false;
    }
    if (transaction) *transaction = found;
    if (derived) *derived = m_derived.listByParent(id, error);
    return true;
}

bool TransactionService::create(Transaction &transaction,
                                QList<DerivedTransaction> &derived,
                                Transaction *out, QString *error)
{
    if (!validateAggregate(transaction, derived, error)) {
        return false;
    }

    if (!m_db.transaction()) {
        if (error) *error = m_db.lastError().text();
        return false;
    }

    if (!m_transactions.add(transaction, error)) {
        m_db.rollback();
        return false;
    }

    for (DerivedTransaction &item : derived) {
        item.transactionId = transaction.id;
        if (!m_derived.add(item, error)) {
            m_db.rollback();
            return false;
        }
    }

    if (!m_db.commit()) {
        if (error) *error = m_db.lastError().text();
        m_db.rollback();
        return false;
    }

    if (out) *out = transaction;
    return true;
}

bool TransactionService::save(const Transaction &transaction,
                              const QList<DerivedTransaction> &derived, QString *error)
{
    if (transaction.id < 0) {
        if (error) *error = QObject::tr("La transaccion no existe.");
        return false;
    }
    if (!validateAggregate(transaction, derived, error)) {
        return false;
    }

    if (!m_db.transaction()) {
        if (error) *error = m_db.lastError().text();
        return false;
    }

    if (!m_transactions.update(transaction, error)) {
        m_db.rollback();
        return false;
    }
    if (!m_derived.deleteByParent(transaction.id, error)) {
        m_db.rollback();
        return false;
    }
    for (const DerivedTransaction &item : derived) {
        DerivedTransaction copy = item;
        copy.id = -1;
        copy.transactionId = transaction.id;
        if (!m_derived.add(copy, error)) {
            m_db.rollback();
            return false;
        }
    }

    if (!m_db.commit()) {
        if (error) *error = m_db.lastError().text();
        m_db.rollback();
        return false;
    }
    return true;
}

bool TransactionService::remove(int id, QString *error)
{
    return m_transactions.remove(id, error);
}

QList<Transaction> TransactionService::all(QString *error) const
{
    return m_transactions.all(error);
}

QHash<int, QList<DerivedTransaction>> TransactionService::allDerived(QString *error) const
{
    return m_derived.allGrouped(error);
}
