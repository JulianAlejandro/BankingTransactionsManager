#ifndef REPORTREPOSITORY_H
#define REPORTREPOSITORY_H

#include <QList>
#include <QSqlDatabase>
#include <QString>

#include "report/reporttypes.h"

// Consultas agregadas de solo lectura para los informes. Trabaja siempre sobre
// derived_transactions unidas a transactions (fecha) y categories (tipo/arbol).
class ReportRepository
{
public:
    explicit ReportRepository(QSqlDatabase db = QSqlDatabase());

    // Ingresos, gastos, neto y numero de movimientos del periodo.
    bool totals(const ReportPeriod &period, PeriodTotals *out, QString *error = nullptr) const;

    // Un CategoryTotal por cada nodo del subarbol con raiz en rootId, con importes
    // directos y acumulados (rama) del periodo.
    bool categoryBreakdown(const ReportPeriod &period, int rootId,
                           QList<CategoryTotal> *out, QString *error = nullptr) const;

    // Evolucion temporal del periodo, agregada segun la granularidad.
    bool timeSeries(const ReportPeriod &period, Granularity granularity,
                    QList<TimeBucket> *out, QString *error = nullptr) const;

private:
    QSqlDatabase m_db;
};

#endif // REPORTREPOSITORY_H
