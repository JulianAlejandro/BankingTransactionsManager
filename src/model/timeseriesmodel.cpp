#include "model/timeseriesmodel.h"

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

TimeSeriesModel::TimeSeriesModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

void TimeSeriesModel::setBuckets(const QList<TimeBucket> &buckets, Granularity granularity)
{
    beginResetModel();
    m_buckets = buckets;
    m_granularity = granularity;
    endResetModel();
}

const QList<TimeBucket> &TimeSeriesModel::buckets() const
{
    return m_buckets;
}

QString TimeSeriesModel::periodLabel(const QDate &start) const
{
    switch (m_granularity) {
    case Granularity::Day:
        return esLocale().toString(start, QLocale::ShortFormat);
    case Granularity::Week:
        return tr("Semana del %1").arg(esLocale().toString(start, QLocale::ShortFormat));
    case Granularity::Month:
        return esLocale().toString(start, QStringLiteral("MMMM yyyy"));
    case Granularity::Quarter:
        return tr("T%1 %2").arg((start.month() - 1) / 3 + 1).arg(start.year());
    case Granularity::Year:
        return QString::number(start.year());
    }
    return QString();
}

int TimeSeriesModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_buckets.size();
}

int TimeSeriesModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant TimeSeriesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_buckets.size()) {
        return QVariant();
    }

    const TimeBucket &bucket = m_buckets.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case PeriodColumn:
            return periodLabel(bucket.start);
        case IncomeColumn:
            return formattedAmount(bucket.incomeCents);
        case ExpenseColumn:
            return formattedAmount(-bucket.expenseCents);
        case NetColumn:
            return formattedAmount(bucket.netCents);
        case CumulativeColumn:
            return formattedAmount(bucket.cumulativeNetCents);
        default:
            break;
        }
        break;
    case Qt::TextAlignmentRole:
        if (index.column() != PeriodColumn) {
            return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
        }
        break;
    case Qt::ForegroundRole:
        if (index.column() == IncomeColumn) {
            return QBrush(QColor(0x1b, 0x7f, 0x3b));
        }
        if (index.column() == ExpenseColumn) {
            return QBrush(QColor(0xb3, 0x26, 0x1e));
        }
        if (index.column() == NetColumn || index.column() == CumulativeColumn) {
            const qint64 value = index.column() == NetColumn ? bucket.netCents
                                                             : bucket.cumulativeNetCents;
            return value >= 0 ? QBrush(QColor(0x1b, 0x7f, 0x3b))
                              : QBrush(QColor(0xb3, 0x26, 0x1e));
        }
        break;
    case CsvRole:
        switch (index.column()) {
        case PeriodColumn:
            return periodLabel(bucket.start);
        case IncomeColumn:
            return csvAmount(bucket.incomeCents);
        case ExpenseColumn:
            return csvAmount(-bucket.expenseCents);
        case NetColumn:
            return csvAmount(bucket.netCents);
        case CumulativeColumn:
            return csvAmount(bucket.cumulativeNetCents);
        default:
            break;
        }
        break;
    default:
        break;
    }

    return QVariant();
}

QVariant TimeSeriesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal) {
        return QVariant();
    }

    switch (section) {
    case PeriodColumn:
        return tr("Periodo");
    case IncomeColumn:
        return tr("Ingresos");
    case ExpenseColumn:
        return tr("Gastos");
    case NetColumn:
        return tr("Neto");
    case CumulativeColumn:
        return tr("Acumulado");
    default:
        break;
    }
    return QVariant();
}
