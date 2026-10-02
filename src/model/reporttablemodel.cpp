#include "model/reporttablemodel.h"

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

QString csvAmount(qint64 cents)
{
    return esLocale().toString(static_cast<double>(cents) / 100.0, 'f', 2);
}

} // namespace

ReportTableModel::ReportTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

void ReportTableModel::setTotals(const QList<CategoryTotal> &totals, bool includeDescendants)
{
    beginResetModel();
    m_totals = totals;
    m_includeDescendants = includeDescendants;
    m_rootCents = totals.isEmpty() ? 0 : totals.first().subtreeCents;
    endResetModel();
}

const QList<CategoryTotal> &ReportTableModel::totals() const
{
    return m_totals;
}

qint64 ReportTableModel::displayedCents(const CategoryTotal &total) const
{
    return m_includeDescendants ? total.subtreeCents : total.directCents;
}

int ReportTableModel::displayedCount(const CategoryTotal &total) const
{
    return m_includeDescendants ? total.subtreeCount : total.directCount;
}

int ReportTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_totals.size();
}

int ReportTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant ReportTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_totals.size()) {
        return QVariant();
    }

    const CategoryTotal &total = m_totals.at(index.row());
    const qint64 cents = displayedCents(total);
    const double share = m_rootCents == 0
        ? 0.0
        : static_cast<double>(cents) / static_cast<double>(m_rootCents) * 100.0;

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case CategoryColumn:
            return total.path;
        case AmountColumn:
            return formattedAmount(cents);
        case ShareColumn:
            return esLocale().toString(share, 'f', 1) + QStringLiteral(" %");
        case CountColumn:
            return displayedCount(total);
        default:
            break;
        }
        break;
    case Qt::TextAlignmentRole:
        if (index.column() != CategoryColumn) {
            return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
        }
        break;
    case Qt::ForegroundRole:
        if (index.column() == AmountColumn) {
            return cents >= 0 ? QBrush(QColor(0x1b, 0x7f, 0x3b))
                              : QBrush(QColor(0xb3, 0x26, 0x1e));
        }
        break;
    case CsvRole:
        switch (index.column()) {
        case CategoryColumn:
            return total.path;
        case AmountColumn:
            return csvAmount(cents);
        case ShareColumn:
            return esLocale().toString(share, 'f', 1);
        case CountColumn:
            return displayedCount(total);
        default:
            break;
        }
        break;
    default:
        break;
    }

    return QVariant();
}

QVariant ReportTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal) {
        return QVariant();
    }

    switch (section) {
    case CategoryColumn:
        return tr("Categoria");
    case AmountColumn:
        return tr("Importe");
    case ShareColumn:
        return tr("%");
    case CountColumn:
        return tr("Movimientos");
    default:
        break;
    }
    return QVariant();
}
