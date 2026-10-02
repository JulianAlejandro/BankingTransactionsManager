#ifndef MOVEMENTTABLEMODEL_H
#define MOVEMENTTABLEMODEL_H

#include <QAbstractTableModel>
#include <QList>

#include "report/reporttypes.h"

class MovementTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column {
        DateColumn = 0,
        ConceptColumn,
        CategoryColumn,
        AmountColumn,
        ColumnCount
    };

    enum Roles {
        CsvRole = Qt::UserRole + 1
    };

    explicit MovementTableModel(QObject *parent = nullptr);

    void setRows(const QList<MovementRow> &rows);
    const QList<MovementRow> &rows() const;
    qint64 totalCents() const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

private:
    QList<MovementRow> m_rows;
};

#endif // MOVEMENTTABLEMODEL_H
