#include "model/derivedtablemodel.h"

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

DerivedTableModel::DerivedTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

void DerivedTableModel::setCategories(const QList<Category> &categories)
{
    m_categories = categories;
    m_categoryById.clear();
    for (const Category &category : m_categories) {
        m_categoryById.insert(category.id, category);
    }
}

void DerivedTableModel::setRows(const QList<DerivedTransaction> &rows)
{
    beginResetModel();
    m_rows = rows;
    endResetModel();
}

const QList<DerivedTransaction> &DerivedTableModel::rows() const
{
    return m_rows;
}

DerivedTransaction DerivedTableModel::derivedAt(int index) const
{
    if (index < 0 || index >= m_rows.size()) {
        return DerivedTransaction();
    }
    return m_rows.at(index);
}

int DerivedTableModel::count() const
{
    return m_rows.size();
}

void DerivedTableModel::appendRow(const DerivedTransaction &row)
{
    const int position = m_rows.size();
    beginInsertRows(QModelIndex(), position, position);
    m_rows.append(row);
    endInsertRows();
}

void DerivedTableModel::replaceRow(int index, const DerivedTransaction &row)
{
    if (index < 0 || index >= m_rows.size()) {
        return;
    }
    m_rows[index] = row;
    const QModelIndex topLeft = this->index(index, 0);
    const QModelIndex bottomRight = this->index(index, ColumnCount - 1);
    emit dataChanged(topLeft, bottomRight);
}

void DerivedTableModel::removeRowAt(int index)
{
    if (index < 0 || index >= m_rows.size()) {
        return;
    }
    beginRemoveRows(QModelIndex(), index, index);
    m_rows.removeAt(index);
    endRemoveRows();
}

qint64 DerivedTableModel::totalCents() const
{
    qint64 total = 0;
    for (const DerivedTransaction &row : m_rows) {
        total += row.amountCents;
    }
    return total;
}

QString DerivedTableModel::categoryPath(int categoryId) const
{
    const Category category = m_categoryById.value(categoryId);
    if (category.id < 0) {
        return QStringLiteral("\u2014");
    }
    return category.path.isEmpty() ? category.name : category.path;
}

int DerivedTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

int DerivedTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant DerivedTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_rows.size()) {
        return QVariant();
    }

    const DerivedTransaction &row = m_rows.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case ConceptColumn:
            return row.concept;
        case CategoryColumn:
            return row.categoryId < 0 ? tr("(sin categoria)") : categoryPath(row.categoryId);
        case AmountColumn:
            return formattedAmount(row.amountCents);
        case NotesColumn:
            return row.notes;
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
    default:
        break;
    }

    return QVariant();
}

QVariant DerivedTableModel::headerData(int section, Qt::Orientation orientation,
                                       int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal) {
        return QVariant();
    }

    switch (section) {
    case ConceptColumn:
        return tr("Concepto");
    case CategoryColumn:
        return tr("Categoria");
    case AmountColumn:
        return tr("Importe");
    case NotesColumn:
        return tr("Notas");
    default:
        break;
    }
    return QVariant();
}
