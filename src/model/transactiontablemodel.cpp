#include "model/transactiontablemodel.h"

#include <QBrush>
#include <QColor>
#include <QLocale>

namespace {

QLocale esLocale()
{
    return QLocale(QLocale::Spanish, QLocale::Spain);
}

QString formattedAmount(qint64 cents)
{
    return esLocale().toCurrencyString(static_cast<double>(cents) / 100.0,
                                       QStringLiteral("\u20ac"));
}

} // namespace

TransactionTableModel::TransactionTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

void TransactionTableModel::setTransactions(
    const QList<Transaction> &transactions,
    const QHash<int, QList<DerivedTransaction>> &derived,
    const QList<Category> &categories)
{
    beginResetModel();
    m_transactions = transactions;
    m_derived = derived;
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

QString TransactionTableModel::classificationFor(const Transaction &transaction) const
{
    const QList<DerivedTransaction> list = m_derived.value(transaction.id);
    if (list.isEmpty()) {
        return tr("Sin clasificar");
    }
    if (list.size() == 1) {
        const Category category = m_categories.value(list.first().categoryId);
        return category.path.isEmpty() ? category.name : category.path;
    }
    return tr("Desglosado (%1)").arg(list.size());
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

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case DateColumn:
            return esLocale().toString(transaction.date, QLocale::ShortFormat);
        case ConceptColumn:
            return transaction.concept;
        case AmountColumn:
            return formattedAmount(transaction.amountCents);
        case AccountColumn:
            return transaction.account;
        case ClassificationColumn:
            return classificationFor(transaction);
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
            return transaction.amountCents >= 0 ? QBrush(QColor(0x1b, 0x7f, 0x3b))
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
    case AmountColumn:
        return tr("Importe");
    case AccountColumn:
        return tr("Cuenta");
    case ClassificationColumn:
        return tr("Clasificacion");
    default:
        break;
    }
    return QVariant();
}
