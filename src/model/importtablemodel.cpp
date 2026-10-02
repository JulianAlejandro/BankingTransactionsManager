#include "model/importtablemodel.h"

#include <algorithm>

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

ImportTableModel::ImportTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

void ImportTableModel::setMovements(const QList<ImportedMovement> &movements)
{
    beginResetModel();
    m_movements = movements;
    m_selected.clear();
    for (int row = 0; row < m_movements.size(); ++row) {
        if (!m_movements.at(row).error.isEmpty() || m_movements.at(row).duplicate) {
            continue;
        }
        m_selected.insert(row);
    }
    endResetModel();
    emit stateChanged();
}

const QList<ImportedMovement> &ImportTableModel::movements() const
{
    return m_movements;
}

bool ImportTableModel::isSelectable(int row) const
{
    if (row < 0 || row >= m_movements.size()) {
        return false;
    }
    const ImportedMovement &movement = m_movements.at(row);
    return movement.error.isEmpty() && !movement.duplicate;
}

QString ImportTableModel::statusText(const ImportedMovement &movement) const
{
    if (!movement.error.isEmpty()) {
        return tr("Error: %1").arg(movement.error);
    }
    if (movement.duplicate) {
        return tr("Duplicado");
    }
    return tr("Nuevo");
}

QList<int> ImportTableModel::selectedIndexes() const
{
    QList<int> indexes = m_selected.values();
    std::sort(indexes.begin(), indexes.end());
    return indexes;
}

void ImportTableModel::selectAll(bool checked)
{
    beginResetModel();
    m_selected.clear();
    if (checked) {
        for (int row = 0; row < m_movements.size(); ++row) {
            if (isSelectable(row)) {
                m_selected.insert(row);
            }
        }
    }
    endResetModel();
    emit stateChanged();
}

int ImportTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_movements.size();
}

int ImportTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant ImportTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_movements.size()) {
        return QVariant();
    }

    const ImportedMovement &movement = m_movements.at(index.row());
    const bool selectable = isSelectable(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case DateColumn:
            return esLocale().toString(movement.date, QLocale::ShortFormat);
        case ConceptColumn:
            return movement.concept;
        case AmountColumn:
            return formattedAmount(movement.amountCents);
        case StatusColumn:
            return statusText(movement);
        default:
            break;
        }
        break;
    case Qt::CheckStateRole:
        if (index.column() == IncludeColumn && selectable) {
            return m_selected.contains(index.row()) ? Qt::Checked : Qt::Unchecked;
        }
        break;
    case Qt::TextAlignmentRole:
        if (index.column() == AmountColumn) {
            return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
        }
        break;
    case Qt::ForegroundRole:
        if (!selectable) {
            return QBrush(QColor(0x99, 0x99, 0x99));
        }
        if (index.column() == AmountColumn) {
            return movement.amountCents >= 0 ? QBrush(QColor(0x1b, 0x7f, 0x3b))
                                             : QBrush(QColor(0xb3, 0x26, 0x1e));
        }
        break;
    default:
        break;
    }

    return QVariant();
}

bool ImportTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::CheckStateRole || index.column() != IncludeColumn
        || !isSelectable(index.row())) {
        return false;
    }

    if (value.toInt() == Qt::Checked) {
        m_selected.insert(index.row());
    } else {
        m_selected.remove(index.row());
    }
    emit dataChanged(index, index, {Qt::CheckStateRole});
    emit stateChanged();
    return true;
}

Qt::ItemFlags ImportTableModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QAbstractTableModel::flags(index);
    if (index.column() == IncludeColumn && isSelectable(index.row())) {
        flags |= Qt::ItemIsUserCheckable;
    }
    return flags;
}

QVariant ImportTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal) {
        return QVariant();
    }

    switch (section) {
    case IncludeColumn:
        return QString();
    case DateColumn:
        return tr("Fecha");
    case ConceptColumn:
        return tr("Concepto");
    case AmountColumn:
        return tr("Importe");
    case StatusColumn:
        return tr("Estado");
    default:
        break;
    }
    return QVariant();
}
