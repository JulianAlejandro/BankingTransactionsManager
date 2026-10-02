#ifndef COMPARISONTABLEMODEL_H
#define COMPARISONTABLEMODEL_H

#include <QAbstractTableModel>

#include "report/reporttypes.h"

class ComparisonTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column {
        MetricColumn = 0,
        AColumn,
        BColumn,
        DeltaColumn,
        PercentColumn,
        ColumnCount
    };

    enum Roles {
        CsvRole = Qt::UserRole + 1
    };

    explicit ComparisonTableModel(QObject *parent = nullptr);

    void setData(const PeriodTotals &a, const PeriodTotals &b);
    const PeriodTotals &a() const;
    const PeriodTotals &b() const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

private:
    static bool isCountRow(int row);
    qint64 valueA(int row) const;
    qint64 valueB(int row) const;

    PeriodTotals m_a;
    PeriodTotals m_b;
};

#endif // COMPARISONTABLEMODEL_H
