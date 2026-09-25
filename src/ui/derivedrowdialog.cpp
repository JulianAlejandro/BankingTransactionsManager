#include "ui/derivedrowdialog.h"
#include "ui_derivedrowdialog.h"

#include <QMessageBox>

DerivedRowDialog::DerivedRowDialog(const QList<Category> &categories, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DerivedRowDialog)
{
    ui->setupUi(this);
    populateCategories(categories);

    connect(ui->comboCategory, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &DerivedRowDialog::updateSignHint);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &DerivedRowDialog::validateAndAccept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    updateSignHint();
}

DerivedRowDialog::~DerivedRowDialog()
{
    delete ui;
}

void DerivedRowDialog::populateCategories(const QList<Category> &categories)
{
    ui->comboCategory->clear();
    for (const Category &category : categories) {
        const QString indent(category.depth * 4, QLatin1Char(' '));
        ui->comboCategory->addItem(indent + category.name, category.id);
        m_typeById.insert(category.id, category.type);
    }
}

void DerivedRowDialog::updateSignHint()
{
    const int categoryId = ui->comboCategory->currentData().toInt();
    const bool income = m_typeById.value(categoryId, CategoryType::Expense) == CategoryType::Income;
    if (income) {
        ui->labelSignHint->setText(tr("+ Ingreso"));
        ui->labelSignHint->setStyleSheet(QStringLiteral("color:#1b7f3b;"));
    } else {
        ui->labelSignHint->setText(tr("- Gasto"));
        ui->labelSignHint->setStyleSheet(QStringLiteral("color:#b3261e;"));
    }
}

void DerivedRowDialog::setRow(const DerivedTransaction &row)
{
    m_row = row;

    if (row.categoryId >= 0) {
        const int index = ui->comboCategory->findData(row.categoryId);
        if (index >= 0) {
            ui->comboCategory->setCurrentIndex(index);
        }
    }
    ui->lineEditConcept->setText(row.concept);
    ui->plainTextEditNotes->setPlainText(row.notes);
    if (row.amountCents != 0) {
        ui->doubleSpinBoxAmount->setValue(static_cast<double>(qAbs(row.amountCents)) / 100.0);
    }
    updateSignHint();
}

void DerivedRowDialog::setSuggestedMagnitudeCents(qint64 cents)
{
    if (ui->doubleSpinBoxAmount->value() == 0.0 && cents > 0) {
        ui->doubleSpinBoxAmount->setValue(static_cast<double>(cents) / 100.0);
    }
}

DerivedTransaction DerivedRowDialog::row() const
{
    DerivedTransaction result = m_row;
    result.categoryId = ui->comboCategory->currentData().toInt();

    const qint64 magnitude = qRound64(ui->doubleSpinBoxAmount->value() * 100.0);
    const CategoryType type = m_typeById.value(result.categoryId, CategoryType::Expense);
    result.amountCents = signedAmountFor(magnitude, type);
    result.concept = ui->lineEditConcept->text().trimmed();
    result.notes = ui->plainTextEditNotes->toPlainText().trimmed();
    return result;
}

void DerivedRowDialog::validateAndAccept()
{
    const int categoryId = ui->comboCategory->currentData().toInt();
    if (categoryId < 0) {
        QMessageBox::warning(this, tr("Datos incompletos"),
                             tr("Selecciona una categoria."));
        return;
    }
    if (qRound64(ui->doubleSpinBoxAmount->value() * 100.0) <= 0) {
        QMessageBox::warning(this, tr("Datos incompletos"),
                             tr("El importe debe ser mayor que cero."));
        return;
    }
    accept();
}
