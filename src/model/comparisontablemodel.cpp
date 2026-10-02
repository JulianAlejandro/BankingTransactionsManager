#include "model/comparisontablemodel.h"

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

ComparisonTableModel::ComparisonTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

void ComparisonTableModel::setData(const PeriodTotals &a, const PeriodTotals &b)
{
    beginResetModel();
    m_a = a;
    m_b = b;
    endResetModel();
}

const PeriodTotals &ComparisonTableModel::a() const
{
    return m_a;
}

const PeriodTotals &ComparisonTableModel::b() const
{
    return m_b;
}

bool ComparisonTableModel::isCountRow(int row)
{
    return row == 3;
}

qint64 ComparisonTableModel::valueA(int row) const
{
    switch (row) {
    case 0:
        return m_a.incomeCents;
    case 1:
        return -m_a.expenseCents;
    case 2:
        return m_a.netCents;
    case 3:
        return m_a.count;
    default:
        return 0;
    }
}

qint64 ComparisonTableModel::valueB(int row) const
{
    switch (row) {
    case 0:
        return m_b.incomeCents;
    case 1:
        return -m_b.expenseCents;
    case 2:
        return m_b.netCents;
    case 3:
        return m_b.count;
    default:
        return 0;
    }
}

int ComparisonTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 4;
}

int ComparisonTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant ComparisonTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= 4) {
        return QVariant();
    }

    const int row = index.row();
    const qint64 a = valueA(row);
    const qint64 b = valueB(row);
    const qint64 delta = a - b;
    const bool count = isCountRow(row);

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case MetricColumn:
            switch (row) {
            case 0: return tr("Ingresos");
            case 1: return tr("Gastos");
            case 2: return tr("Neto");
            case 3: return tr("Movimientos");
            default: break;
            }
            break;
        case AColumn:
            return count ? QString::number(a) : formattedAmount(a);
        case BColumn:
            return count ? QString::number(b) : formattedAmount(b);
        case DeltaColumn:
            return count ? QString::number(delta) : formattedAmount(delta);
        case PercentColumn:
            if (b == 0) {
                return QStringLiteral("\u2014");
            }
            return esLocale().toString(static_cast<double>(delta) / qAbs(b) * 100.0, 'f', 1)
                + QStringLiteral(" %");
        default:
            break;
        }
        break;
    case Qt::TextAlignmentRole:
        if (index.column() != MetricColumn) {
            return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
        }
        break;
    case Qt::ForegroundRole:
        if (index.column() == DeltaColumn) {
            return delta >= 0 ? QBrush(QColor(0x1b, 0x7f, 0x3b))
                              : QBrush(QColor(0xb3, 0x26, 0x1e));
        }
        break;
    case CsvRole:
        switch (index.column()) {
        case MetricColumn:
            switch (row) {
            case 0: return tr("Ingresos");
            case 1: return tr("Gastos");
            case 2: return tr("Neto");
            case 3: return tr("Movimientos");
            default: break;
            }
            break;
        case AColumn:
            return QString::number(a);
        case BColumn:
            return QString::number(b);
        case DeltaColumn:
            return QString::number(delta);
        case PercentColumn:
            if (b == 0) {
                return QString();
            }
            return esLocale().toString(static_cast<double>(delta) / qAbs(b) * 100.0, 'f', 1);
        default:
            break;
        }
        break;
    default:
        break;
    }

    return QVariant();
}

QVariant ComparisonTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal) {
        return QVariant();
    }

    switch (section) {
    case MetricColumn:
        return tr("Metrica");
    case AColumn:
        return tr("Periodo A");
    case BColumn:
        return tr("Periodo B");
    case DeltaColumn:
        return tr("Variacion");
    case PercentColumn:
        return tr("%");
    default:
        break;
    }
    return QVariant();
}
