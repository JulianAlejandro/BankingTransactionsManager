#ifndef TRANSACTIONSERVICE_H
#define TRANSACTIONSERVICE_H

#include <QHash>
#include <QList>
#include <QSqlDatabase>
#include <QString>

#include "data/categoryrepository.h"
#include "data/derivedtransactionrepository.h"
#include "data/transactionrepository.h"
#include "model/derivedtransaction.h"
#include "model/transaction.h"

// Coordina el agregado "transaccion + derivadas" manteniendo las invariantes:
//   - al menos una transaccion derivada por transaccion,
//   - la suma de las derivadas coincide con el importe del padre,
//   - el signo de cada derivada es acorde al tipo de su categoria.
class TransactionService
{
public:
    explicit TransactionService(QSqlDatabase db = QSqlDatabase());

    bool load(int id, Transaction *transaction, QList<DerivedTransaction> *derived,
              QString *error = nullptr) const;
    bool create(Transaction &transaction, QList<DerivedTransaction> &derived,
                Transaction *out = nullptr, QString *error = nullptr);
    bool save(const Transaction &transaction, const QList<DerivedTransaction> &derived,
              QString *error = nullptr);
    bool remove(int id, QString *error = nullptr);

    QList<Transaction> all(QString *error = nullptr) const;
    QHash<int, QList<DerivedTransaction>> allDerived(QString *error = nullptr) const;

    bool validateAggregate(const Transaction &transaction,
                           const QList<DerivedTransaction> &derived,
                           QString *error = nullptr) const;

private:
    QSqlDatabase m_db;
    TransactionRepository m_transactions;
    DerivedTransactionRepository m_derived;
    CategoryRepository m_categories;
};

#endif // TRANSACTIONSERVICE_H
