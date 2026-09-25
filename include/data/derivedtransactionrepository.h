#ifndef DERIVEDTRANSACTIONREPOSITORY_H
#define DERIVEDTRANSACTIONREPOSITORY_H

#include <QHash>
#include <QList>
#include <QSqlDatabase>
#include <QString>

#include "model/derivedtransaction.h"

class DerivedTransactionRepository
{
public:
    explicit DerivedTransactionRepository(QSqlDatabase db = QSqlDatabase());

    QList<DerivedTransaction> listByParent(int transactionId, QString *error = nullptr) const;
    QHash<int, QList<DerivedTransaction>> allGrouped(QString *error = nullptr) const;

    bool add(DerivedTransaction &derived, QString *error = nullptr);
    bool update(const DerivedTransaction &derived, QString *error = nullptr);
    bool remove(int id, QString *error = nullptr);
    bool deleteByParent(int transactionId, QString *error = nullptr);

private:
    QSqlDatabase m_db;
};

#endif // DERIVEDTRANSACTIONREPOSITORY_H
