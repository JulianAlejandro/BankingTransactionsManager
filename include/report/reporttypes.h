#ifndef REPORTTYPES_H
#define REPORTTYPES_H

#include <QDate>
#include <QString>

#include "model/category.h"

enum class Granularity {
    Day,
    Week,
    Month,
    Quarter,
    Year
};

struct ReportPeriod
{
    QDate from;
    QDate to;

    bool isValid() const
    {
        return from.isValid() && to.isValid() && from <= to;
    }
};

struct PeriodTotals
{
    qint64 incomeCents = 0;    // ingreso > 0
    qint64 expenseCents = 0;   // gasto < 0
    qint64 netCents = 0;       // incomeCents + expenseCents
    int count = 0;             // numero de transacciones en el periodo
};

struct CategoryTotal
{
    int categoryId = -1;
    QString path;
    CategoryType type = CategoryType::Expense;
    qint64 directCents = 0;    // solo este nodo
    qint64 subtreeCents = 0;   // este nodo mas sus descendientes
    int directCount = 0;       // derivadas en este nodo
    int subtreeCount = 0;      // derivadas en el subarbol
};

struct TimeBucket
{
    QDate start;               // inicio del bucket (dia/semana/mes/trimestre/ano)
    qint64 incomeCents = 0;
    qint64 expenseCents = 0;
    qint64 netCents = 0;
    qint64 cumulativeNetCents = 0;   // saldo acumulado hasta este bucket
};

#endif // REPORTTYPES_H
