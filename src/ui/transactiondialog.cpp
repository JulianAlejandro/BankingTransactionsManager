#include "ui/transactiondialog.h"
#include "ui_transactiondialog.h"

#include <QMessageBox>

TransactionDialog::TransactionDialog(const QList<Category> &categories, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::TransactionDialog)
{
    ui->setupUi(this);
    populateCategories(categories);

    ui->dateEdit->setDate(QDate::currentDate());
    ui->comboCategory->setFocus();

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &TransactionDialog::validateAndAccept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

TransactionDialog::~TransactionDialog()
{
    delete ui;
}

void TransactionDialog::populateCategories(const QList<Category> &categories)
{
    ui->comboCategory->clear();
    for (const Category &category : categories) {
        const QString indent(category.depth * 4, QLatin1Char(' '));
        ui->comboCategory->addItem(indent + category.name, category.id);
    }
}

void TransactionDialog::setTransaction(const Transaction &transaction)
{
    m_transaction = transaction;

    ui->dateEdit->setDate(transaction.date);
    ui->lineEditConcept->setText(transaction.concept);

    const int index = ui->comboCategory->findData(transaction.categoryId);
    if (index >= 0) {
        ui->comboCategory->setCurrentIndex(index);
    }

    ui->doubleSpinBoxAmount->setValue(static_cast<double>(transaction.amountCents) / 100.0);
    ui->lineEditAccount->setText(transaction.account);
    ui->plainTextEditNotes->setPlainText(transaction.notes);
}

Transaction TransactionDialog::transaction() const
{
    Transaction transaction = m_transaction;
    transaction.date = ui->dateEdit->date();
    transaction.concept = ui->lineEditConcept->text().trimmed();
    transaction.categoryId = ui->comboCategory->currentData().toInt();
    transaction.amountCents = qRound64(ui->doubleSpinBoxAmount->value() * 100.0);
    transaction.account = ui->lineEditAccount->text().trimmed();
    transaction.notes = ui->plainTextEditNotes->toPlainText().trimmed();
    return transaction;
}

void TransactionDialog::validateAndAccept()
{
    const Transaction candidate = transaction();

    QString error;
    if (!candidate.isValid(&error)) {
        QMessageBox::warning(this, tr("Datos incompletos"), error);
        return;
    }
    accept();
}
