#ifndef REPORTTABLEMODEL_H
#define REPORTTABLEMODEL_H

#include <QAbstractTableModel>
#include <QList>

#include "report/reporttypes.h"

class ReportTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column {
        CategoryColumn = 0,
        AmountColumn,
        ShareColumn,
        CountColumn,
        ColumnCount
    };

    enum Roles {
        CsvRole = Qt::UserRole + 1
    };

    explicit ReportTableModel(QObject *parent = nullptr);

    void setTotals(const QList<CategoryTotal> &totals, bool includeDescendants);
    const QList<CategoryTotal> &totals() const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

private:
    qint64 displayedCents(const CategoryTotal &total) const;
    int displayedCount(const CategoryTotal &total) const;

    QList<CategoryTotal> m_totals;
    bool m_includeDescendants = true;
    qint64 m_rootCents = 0;
};

#endif // REPORTTABLEMODEL_H
