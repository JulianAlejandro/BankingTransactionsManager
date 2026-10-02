#include "ui/movementlistdialog.h"

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QLocale>
#include <QTableView>
#include <QVBoxLayout>

#include "model/movementtablemodel.h"

namespace {

QString formattedAmount(qint64 cents)
{
    const QLocale locale(QLocale::Spanish, QLocale::Spain);
    return locale.toCurrencyString(static_cast<double>(cents) / 100.0,
                                   QStringLiteral("\u20ac"));
}

} // namespace

MovementListDialog::MovementListDialog(const QString &title, const QList<MovementRow> &rows,
                                       QWidget *parent)
    : QDialog(parent)
    , m_model(new MovementTableModel(this))
    , m_view(new QTableView(this))
{
    setWindowTitle(title);
    m_model->setRows(rows);

    m_view->setModel(m_model);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_view->setAlternatingRowColors(false);
    m_view->verticalHeader()->setVisible(false);
    m_view->horizontalHeader()->setSectionResizeMode(
        MovementTableModel::ConceptColumn, QHeaderView::Stretch);
    m_view->horizontalHeader()->setStretchLastSection(true);

    QLabel *summary = new QLabel(
        tr("%1 movimientos \u00b7 total %2").arg(rows.size()).arg(formattedAmount(m_model->totalCents())),
        this);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(summary);
    layout->addWidget(m_view, 1);
    layout->addWidget(buttons);

    resize(600, 420);
}
