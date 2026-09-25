#ifndef DERIVEDTRANSACTION_H
#define DERIVEDTRANSACTION_H

#include <QString>

#include "model/category.h"

struct DerivedTransaction
{
    int id = -1;
    int transactionId = -1;
    int categoryId = -1;
    QString concept;
    qint64 amountCents = 0;   // firmado segun la categoria
    QString notes;

    bool isValid(QString *error = nullptr) const;
};

inline qint64 signedAmountFor(qint64 magnitudeCents, CategoryType type)
{
    return type == CategoryType::Income ? magnitudeCents : -magnitudeCents;
}

#endif // DERIVEDTRANSACTION_H
