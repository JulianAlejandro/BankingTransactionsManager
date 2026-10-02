#include "model/movementtablemodel.h"

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

MovementTableModel::MovementTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

void MovementTableModel::setRows(const QList<MovementRow> &rows)
{
    beginResetModel();
    m_rows = rows;
    endResetModel();
}

const QList<MovementRow> &MovementTableModel::rows() const
{
    return m_rows;
}

qint64 MovementTableModel::totalCents() const
{
    qint64 total = 0;
    for (const MovementRow &row : m_rows) {
        total += row.amountCents;
    }
    return total;
}

int MovementTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

int MovementTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant MovementTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_rows.size()) {
        return QVariant();
    }

    const MovementRow &row = m_rows.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case DateColumn:
            return esLocale().toString(row.date, QLocale::ShortFormat);
        case ConceptColumn:
            return row.concept;
        case CategoryColumn:
            return row.categoryPath;
        case AmountColumn:
            return formattedAmount(row.amountCents);
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
            return row.amountCents >= 0 ? QBrush(QColor(0x1b, 0x7f, 0x3b))
                                        : QBrush(QColor(0xb3, 0x26, 0x1e));
        }
        break;
    case CsvRole:
        switch (index.column()) {
        case DateColumn:
            return row.date.toString(Qt::ISODate);
        case ConceptColumn:
            return row.concept;
        case CategoryColumn:
            return row.categoryPath;
        case AmountColumn:
            return csvAmount(row.amountCents);
        default:
            break;
        }
        break;
    default:
        break;
    }

    return QVariant();
}

QVariant MovementTableModel::headerData(int section, Qt::Orientation orientation, int role) const
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
    default:
        break;
    }
    return QVariant();
}
