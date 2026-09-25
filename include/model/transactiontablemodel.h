#ifndef TRANSACTIONTABLEMODEL_H
#define TRANSACTIONTABLEMODEL_H

#include <QAbstractTableModel>
#include <QHash>
#include <QList>

#include "model/category.h"
#include "model/derivedtransaction.h"
#include "model/transaction.h"

class TransactionTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column {
        DateColumn = 0,
        ConceptColumn,
        AmountColumn,
        AccountColumn,
        ClassificationColumn,
        ColumnCount
    };

    explicit TransactionTableModel(QObject *parent = nullptr);

    void setTransactions(const QList<Transaction> &transactions,
                         const QHash<int, QList<DerivedTransaction>> &derived,
                         const QList<Category> &categories);
    bool transactionAt(int row, Transaction *out = nullptr) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

private:
    QString classificationFor(const Transaction &transaction) const;
    QList<Transaction> m_transactions;
    QHash<int, QList<DerivedTransaction>> m_derived;
    QHash<int, Category> m_categories;
};

#endif // TRANSACTIONTABLEMODEL_H
