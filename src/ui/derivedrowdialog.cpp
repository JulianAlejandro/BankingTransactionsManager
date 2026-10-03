#include "ui/derivedrowdialog.h"
#include "ui_derivedrowdialog.h"

#include <QMessageBox>
#include <QPushButton>

#include "data/categoryrepository.h"
#include "ui/categoryeditordialog.h"

DerivedRowDialog::DerivedRowDialog(CategoryRepository *repository, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DerivedRowDialog)
    , m_repository(repository)
{
    ui->setupUi(this);
    reloadCategories();

    connect(ui->comboCategory, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &DerivedRowDialog::updateSignHint);
    connect(ui->btnNewCategory, &QPushButton::clicked, this, &DerivedRowDialog::createCategory);
    connect(ui->btnLoadParent, &QPushButton::clicked, this, &DerivedRowDialog::loadFromParent);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &DerivedRowDialog::validateAndAccept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    updateSignHint();
}

DerivedRowDialog::~DerivedRowDialog()
{
    delete ui;
}

void DerivedRowDialog::reloadCategories(int selectId)
{
    ui->comboCategory->clear();
    m_typeById.clear();

    if (m_repository == nullptr) {
        return;
    }

    QString error;
    const QList<Category> categories = m_repository->withPaths(&error);
    for (const Category &category : categories) {
        const QString indent(category.depth * 4, QLatin1Char(' '));
        ui->comboCategory->addItem(indent + category.name, category.id);
        m_typeById.insert(category.id, category.type);
    }

    if (selectId >= 0) {
        const int index = ui->comboCategory->findData(selectId);
        if (index >= 0) {
            ui->comboCategory->setCurrentIndex(index);
        }
    }
    updateSignHint();
}

Category DerivedRowDialog::currentCategory() const
{
    const int id = ui->comboCategory->currentData().toInt();
    Category category;
    category.id = id;
    category.type = m_typeById.value(id, CategoryType::Expense);
    category.name = ui->comboCategory->currentText().trimmed();
    return category;
}

void DerivedRowDialog::createCategory()
{
    if (m_repository == nullptr) {
        return;
    }

    const int parentId = ui->comboCategory->currentData().toInt();
    if (parentId < 0) {
        QMessageBox::information(this, tr("Nueva categoria"),
                                 tr("Selecciona antes una categoria padre."));
        return;
    }

    bool parentOk = false;
    const Category parentCategory = m_repository->findById(parentId, &parentOk);
    if (!parentOk) {
        QMessageBox::critical(this, tr("Nueva categoria"),
                              tr("La categoria padre no existe."));
        return;
    }

    QString error;
    const QString path = m_repository->pathOf(parentId, &parentOk);
    Category parent = parentCategory;
    if (parentOk) {
        parent.path = path;
    }

    CategoryEditorDialog dialog(m_repository, parent, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    reloadCategories(dialog.createdCategory().id);
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

void DerivedRowDialog::setParentInfo(const QString &concept, qint64 amountCents,
                                     const QString &notes)
{
    m_hasParent = true;
    m_parentConcept = concept;
    m_parentAmountCents = amountCents;
    m_parentNotes = notes;
    ui->btnLoadParent->setEnabled(true);
}

void DerivedRowDialog::loadFromParent()
{
    if (!m_hasParent) {
        return;
    }

    ui->lineEditConcept->setText(m_parentConcept);
    ui->plainTextEditNotes->setPlainText(m_parentNotes);
    if (m_parentAmountCents != 0) {
        ui->doubleSpinBoxAmount->setValue(static_cast<double>(qAbs(m_parentAmountCents)) / 100.0);
    }

    const int categoryId = ui->comboCategory->currentData().toInt();
    if (categoryId >= 0) {
        const bool parentIncome = m_parentAmountCents > 0;
        const bool categoryIncome =
            m_typeById.value(categoryId, CategoryType::Expense) == CategoryType::Income;
        if (parentIncome != categoryIncome) {
            QMessageBox::warning(
                this, tr("Signo incompatible"),
                tr("Has cargado el importe del padre, pero la categoria seleccionada es de %1 "
                   "y el movimiento bancario es de %2. El importe no cuadrara: elige una "
                   "categoria del mismo signo o ajusta el importe.")
                    .arg(categoryIncome ? tr("ingreso") : tr("gasto"),
                         parentIncome ? tr("ingreso") : tr("gasto")));
        }
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
