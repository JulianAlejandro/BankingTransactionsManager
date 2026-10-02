#ifndef TIMESERIESMODEL_H
#define TIMESERIESMODEL_H

#include <QAbstractTableModel>
#include <QList>

#include "report/reporttypes.h"

class TimeSeriesModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column {
        PeriodColumn = 0,
        IncomeColumn,
        ExpenseColumn,
        NetColumn,
        CumulativeColumn,
        ColumnCount
    };

    enum Roles {
        CsvRole = Qt::UserRole + 1
    };

    explicit TimeSeriesModel(QObject *parent = nullptr);

    void setBuckets(const QList<TimeBucket> &buckets, Granularity granularity);
    const QList<TimeBucket> &buckets() const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

private:
    QString periodLabel(const QDate &start) const;

    QList<TimeBucket> m_buckets;
    Granularity m_granularity = Granularity::Month;
};

#endif // TIMESERIESMODEL_H
