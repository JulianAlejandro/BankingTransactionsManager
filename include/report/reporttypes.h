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

struct MovementRow
{
    int transactionId = -1;
    int derivedId = -1;
    int categoryId = -1;
    QDate date;
    QString concept;
    QString categoryPath;
    qint64 amountCents = 0;    // firmado segun la categoria
    QString account;
};

// Periodo de igual duracion inmediatamente anterior.
inline ReportPeriod previousPeriod(const ReportPeriod &period)
{
    if (!period.isValid()) {
        return {};
    }
    const qint64 days = period.from.daysTo(period.to);
    const QDate to = period.from.addDays(-1);
    return {to.addDays(-days), to};
}

// Mismo rango desplazado un ano atras.
inline ReportPeriod samePeriodLastYear(const ReportPeriod &period)
{
    if (!period.isValid()) {
        return {};
    }
    return {period.from.addYears(-1), period.to.addYears(-1)};
}

#endif // REPORTTYPES_H
