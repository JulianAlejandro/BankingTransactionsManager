#include "model/transactiontablemodel.h"

#include <QBrush>
#include <QColor>
#include <QLocale>

namespace {

QLocale esLocale()
{
    return QLocale(QLocale::Spanish, QLocale::Spain);
}

QString formattedAmount(const Transaction &transaction, const Category &category)
{
    const double value = static_cast<double>(transaction.amountCents) / 100.0;
    const QString text = esLocale().toCurrencyString(value, QStringLiteral("\u20ac"));
    return category.type == CategoryType::Income ? QStringLiteral("+") + text
                                                : QStringLiteral("-") + text;
}

} // namespace

TransactionTableModel::TransactionTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

void TransactionTableModel::setTransactions(const QList<Transaction> &transactions,
                                            const QList<Category> &categories)
{
    beginResetModel();
    m_transactions = transactions;
    m_categories.clear();
    for (const Category &category : categories) {
        m_categories.insert(category.id, category);
    }
    endResetModel();
}

bool TransactionTableModel::transactionAt(int row, Transaction *out) const
{
    if (row < 0 || row >= m_transactions.size()) {
        return false;
    }
    if (out) {
        *out = m_transactions.at(row);
    }
    return true;
}

int TransactionTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_transactions.size();
}

int TransactionTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant TransactionTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_transactions.size()) {
        return QVariant();
    }

    const Transaction &transaction = m_transactions.at(index.row());
    const Category category = m_categories.value(transaction.categoryId);

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case DateColumn:
            return esLocale().toString(transaction.date, QLocale::ShortFormat);
        case ConceptColumn:
            return transaction.concept;
        case CategoryColumn:
            return category.path.isEmpty() ? category.name : category.path;
        case AmountColumn:
            return formattedAmount(transaction, category);
        case AccountColumn:
            return transaction.account;
        default:
            break;
        }
        break;
    case Qt::TextAlignmentRole:
        if (index.column() == AmountColumn) {
            return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
        }
        break;
    case Qt::ForegroundRole:
        if (index.column() == AmountColumn) {
            return category.type == CategoryType::Income
                ? QBrush(QColor(0x1b, 0x7f, 0x3b))
                : QBrush(QColor(0xb3, 0x26, 0x1e));
        }
        break;
    case Qt::UserRole:
        return transaction.id;
    default:
        break;
    }

    return QVariant();
}

QVariant TransactionTableModel::headerData(int section, Qt::Orientation orientation,
                                           int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal) {
        return QVariant();
    }

    switch (section) {
    case DateColumn:
        return tr("Fecha");
    case ConceptColumn:
        return tr("Concepto");
    case CategoryColumn:
        return tr("Categoria");
    case AmountColumn:
        return tr("Importe");
    case AccountColumn:
        return tr("Cuenta");
    default:
        break;
    }
    return QVariant();
}
