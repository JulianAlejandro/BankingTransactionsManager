#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <QDate>
#include <QString>

struct Transaction
{
    int id = -1;
    QDate date;
    QString concept;
    qint64 amountCents = 0;   // firmado: ingreso > 0, gasto < 0
    QString account;
    QString notes;

    bool isValid(QString *error = nullptr) const;
};

#endif // TRANSACTION_H
