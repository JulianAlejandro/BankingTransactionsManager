#include "ui/transactioneditordialog.h"
#include "ui_transactioneditordialog.h"

#include <QItemSelectionModel>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>

#include "ui/derivedrowdialog.h"

namespace {

QLocale esLocale()
{
    return QLocale(QLocale::Spanish, QLocale::Spain);
}

QString formatCents(qint64 cents)
{
    return esLocale().toCurrencyString(static_cast<double>(cents) / 100.0,
                                       QStringLiteral("\u20ac"));
}

} // namespace

TransactionEditorDialog::TransactionEditorDialog(const QList<Category> &categories, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::TransactionEditorDialog)
    , m_model(new DerivedTableModel(this))
    , m_categories(categories)
{
    ui->setupUi(this);

    ui->tableViewDerived->setModel(m_model);
    ui->tableViewDerived->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableViewDerived->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableViewDerived->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableViewDerived->horizontalHeader()->setStretchLastSection(true);
    ui->tableViewDerived->verticalHeader()->setVisible(false);

    m_model->setCategories(categories);
    ui->dateEdit->setDate(QDate::currentDate());

    connect(ui->buttonBox, &QDialogButtonBox::accepted,
            this, &TransactionEditorDialog::validateAndAccept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(ui->btnAddDerived, &QPushButton::clicked, this, &TransactionEditorDialog::addDerived);
    connect(ui->btnEditDerived, &QPushButton::clicked, this, &TransactionEditorDialog::editDerived);
    connect(ui->btnRemoveDerived, &QPushButton::clicked, this, &TransactionEditorDialog::removeDerived);
    connect(ui->doubleSpinBoxAmount, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &TransactionEditorDialog::refreshState);
    connect(ui->lineEditConcept, &QLineEdit::textChanged,
            this, &TransactionEditorDialog::refreshState);
    connect(ui->dateEdit, &QDateEdit::dateChanged,
            this, &TransactionEditorDialog::refreshState);
    connect(ui->tableViewDerived->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &TransactionEditorDialog::refreshState);

    DerivedTransaction defaultRow;
    m_model->appendRow(defaultRow);

    refreshState();
}

TransactionEditorDialog::~TransactionEditorDialog()
{
    delete ui;
}

void TransactionEditorDialog::setTransaction(const Transaction &transaction,
                                             const QList<DerivedTransaction> &derived)
{
    m_transaction = transaction;
    ui->dateEdit->setDate(transaction.date);
    ui->lineEditConcept->setText(transaction.concept);
    ui->doubleSpinBoxAmount->setValue(static_cast<double>(transaction.amountCents) / 100.0);
    ui->lineEditAccount->setText(transaction.account);
    ui->plainTextEditNotes->setPlainText(transaction.notes);

    m_model->setRows(derived);
    refreshState();
}

qint64 TransactionEditorDialog::amountCents() const
{
    return qRound64(ui->doubleSpinBoxAmount->value() * 100.0);
}

int TransactionEditorDialog::selectedRow() const
{
    const QModelIndexList selection = ui->tableViewDerived->selectionModel()->selectedRows();
    return selection.isEmpty() ? -1 : selection.first().row();
}

Transaction TransactionEditorDialog::transaction() const
{
    Transaction result = m_transaction;
    result.date = ui->dateEdit->date();
    result.concept = ui->lineEditConcept->text().trimmed();
    result.amountCents = amountCents();
    result.account = ui->lineEditAccount->text().trimmed();
    result.notes = ui->plainTextEditNotes->toPlainText().trimmed();
    return result;
}

QList<DerivedTransaction> TransactionEditorDialog::derivedTransactions() const
{
    return m_model->rows();
}

void TransactionEditorDialog::addDerived()
{
    DerivedTransaction row;
    row.concept = ui->lineEditConcept->text().trimmed();
    row.notes = ui->plainTextEditNotes->toPlainText().trimmed();

    DerivedRowDialog dialog(m_categories, this);
    dialog.setRow(row);
    const qint64 remaining = amountCents() - m_model->totalCents();
    dialog.setSuggestedMagnitudeCents(qAbs(remaining));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    m_model->appendRow(dialog.row());
    refreshState();
}

void TransactionEditorDialog::editDerived()
{
    const int index = selectedRow();
    if (index < 0) {
        QMessageBox::information(this, tr("Desglose"),
                                 tr("Selecciona una transaccion derivada."));
        return;
    }

    const DerivedTransaction current = m_model->derivedAt(index);
    DerivedRowDialog dialog(m_categories, this);
    dialog.setRow(current);
    const qint64 remaining = amountCents() - (m_model->totalCents() - current.amountCents);
    dialog.setSuggestedMagnitudeCents(qAbs(remaining));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    m_model->replaceRow(index, dialog.row());
    refreshState();
}

void TransactionEditorDialog::removeDerived()
{
    const int index = selectedRow();
    if (index < 0) {
        QMessageBox::information(this, tr("Desglose"),
                                 tr("Selecciona una transaccion derivada."));
        return;
    }
    if (m_model->count() <= 1) {
        QMessageBox::information(this, tr("Desglose"),
                                 tr("Debe existir al menos una transaccion derivada."));
        return;
    }
    m_model->removeRowAt(index);
    refreshState();
}

bool TransactionEditorDialog::validateState(QString *error) const
{
    const Transaction candidate = transaction();
    if (!candidate.isValid(error)) {
        return false;
    }
    if (m_model->count() < 1) {
        if (error) *error = tr("Debe existir al menos una transaccion derivada.");
        return false;
    }
    for (const DerivedTransaction &row : m_model->rows()) {
        if (row.categoryId < 0) {
            if (error) *error = tr("Cada derivada debe tener una categoria.");
            return false;
        }
        if (row.amountCents == 0) {
            if (error) *error = tr("El importe de cada derivada debe ser distinto de cero.");
            return false;
        }
    }
    if (m_model->totalCents() != candidate.amountCents) {
        if (error) *error = tr("La suma de las derivadas no coincide con el importe.");
        return false;
    }
    return true;
}

void TransactionEditorDialog::refreshState()
{
    const qint64 difference = amountCents() - m_model->totalCents();

    ui->labelSum->setText(tr("Suma derivadas: %1").arg(formatCents(m_model->totalCents())));
    ui->labelDifference->setText(tr("Diferencia: %1").arg(formatCents(difference)));
    ui->labelDifference->setStyleSheet(difference == 0 ? QStringLiteral("color:#1b7f3b;")
                                                       : QStringLiteral("color:#b3261e;"));
    ui->btnRemoveDerived->setEnabled(m_model->count() > 1);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(validateState(nullptr));
}

void TransactionEditorDialog::validateAndAccept()
{
    QString error;
    if (!validateState(&error)) {
        QMessageBox::warning(this, tr("Datos incompletos"), error);
        return;
    }
    accept();
}
