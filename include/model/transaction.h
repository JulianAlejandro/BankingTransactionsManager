#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <QDate>
#include <QString>

struct Transaction
{
    int id = -1;
    QDate date;
    QString concept;
    int categoryId = -1;
    qint64 amountCents = 0;
    QString account;
    QString notes;

    bool isValid(QString *error = nullptr) const;
};

#endif // TRANSACTION_H
