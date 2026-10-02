#ifndef IMPORTTABLEMODEL_H
#define IMPORTTABLEMODEL_H

#include <QAbstractTableModel>
#include <QList>
#include <QSet>

#include "data/importtypes.h"

class ImportTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column {
        IncludeColumn = 0,
        DateColumn,
        ConceptColumn,
        AmountColumn,
        StatusColumn,
        ColumnCount
    };

    explicit ImportTableModel(QObject *parent = nullptr);

    void setMovements(const QList<ImportedMovement> &movements);
    const QList<ImportedMovement> &movements() const;
    QList<int> selectedIndexes() const;
    void selectAll(bool checked);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

signals:
    void stateChanged();

private:
    bool isSelectable(int row) const;
    QString statusText(const ImportedMovement &movement) const;

    QList<ImportedMovement> m_movements;
    QSet<int> m_selected;
};

#endif // IMPORTTABLEMODEL_H
