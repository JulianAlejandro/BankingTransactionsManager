#ifndef TRANSACTIONREPOSITORY_H
#define TRANSACTIONREPOSITORY_H

#include <QList>
#include <QSqlDatabase>
#include <QString>

#include "model/transaction.h"

class TransactionRepository
{
public:
    explicit TransactionRepository(QSqlDatabase db = QSqlDatabase());

    bool add(Transaction &transaction, QString *error = nullptr);
    bool update(const Transaction &transaction, QString *error = nullptr);
    bool remove(int id, QString *error = nullptr);
    QList<Transaction> all(QString *error = nullptr) const;

private:
    QSqlDatabase m_db;
};

#endif // TRANSACTIONREPOSITORY_H
