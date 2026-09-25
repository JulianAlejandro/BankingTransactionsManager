#ifndef DERIVEDTABLEMODEL_H
#define DERIVEDTABLEMODEL_H

#include <QAbstractTableModel>
#include <QHash>
#include <QList>

#include "model/category.h"
#include "model/derivedtransaction.h"

class DerivedTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column {
        ConceptColumn = 0,
        CategoryColumn,
        AmountColumn,
        NotesColumn,
        ColumnCount
    };

    explicit DerivedTableModel(QObject *parent = nullptr);

    void setCategories(const QList<Category> &categories);
    void setRows(const QList<DerivedTransaction> &rows);

    const QList<DerivedTransaction> &rows() const;
    DerivedTransaction derivedAt(int index) const;
    int count() const;
    void appendRow(const DerivedTransaction &row);
    void replaceRow(int index, const DerivedTransaction &row);
    void removeRowAt(int index);
    qint64 totalCents() const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

private:
    QString categoryPath(int categoryId) const;
    QList<Category> m_categories;
    QHash<int, Category> m_categoryById;
    QList<DerivedTransaction> m_rows;
};

#endif // DERIVEDTABLEMODEL_H
