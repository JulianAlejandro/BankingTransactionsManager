#include "ui/importdialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>

#include "data/categoryrepository.h"
#include "data/importservice.h"
#include "model/importtablemodel.h"

ImportDialog::ImportDialog(ImportService *service, CategoryRepository *categories,
                           QWidget *parent)
    : QDialog(parent)
    , m_service(service)
    , m_categories(categories)
    , m_model(new ImportTableModel(this))
    , m_path(new QLineEdit(this))
    , m_view(new QTableView(this))
    , m_category(new QComboBox(this))
    , m_selectAll(new QCheckBox(tr("Seleccionar todo"), this))
    , m_summary(new QLabel(this))
    , m_importButton(new QPushButton(tr("Importar"), this))
{
    setWindowTitle(tr("Importar movimientos"));
    m_path->setReadOnly(true);

    QPushButton *browseButton = new QPushButton(tr("Examinar..."), this);
    QHBoxLayout *fileLayout = new QHBoxLayout();
    fileLayout->addWidget(m_path, 1);
    fileLayout->addWidget(browseButton);

    m_view->setModel(m_model);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_view->setAlternatingRowColors(false);
    m_view->verticalHeader()->setVisible(false);
    m_view->horizontalHeader()->setSectionResizeMode(
        ImportTableModel::ConceptColumn, QHeaderView::Stretch);
    m_view->horizontalHeader()->setStretchLastSection(true);
    m_view->setColumnWidth(ImportTableModel::IncludeColumn, 28);

    QHBoxLayout *categoryLayout = new QHBoxLayout();
    categoryLayout->addWidget(new QLabel(tr("Categoria destino"), this));
    categoryLayout->addWidget(m_category, 1);
    categoryLayout->addWidget(m_selectAll);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    buttons->addButton(m_importButton, QDialogButtonBox::AcceptRole);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addLayout(fileLayout);
    layout->addWidget(m_view, 1);
    layout->addLayout(categoryLayout);
    layout->addWidget(m_summary);
    layout->addWidget(buttons);

    populateCategories();

    connect(browseButton, &QPushButton::clicked, this, &ImportDialog::browse);
    connect(m_model, &ImportTableModel::stateChanged, this, &ImportDialog::updateSummary);
    connect(m_selectAll, &QCheckBox::toggled, this, [this](bool checked) {
        m_model->selectAll(checked);
    });
    connect(m_importButton, &QPushButton::clicked, this, &ImportDialog::runImport);

    updateSummary();
    resize(760, 480);
}

int ImportDialog::importedCount() const
{
    return m_imported;
}

void ImportDialog::populateCategories()
{
    QString error;
    const QList<Category> tree = m_categories->withPaths(&error);
    for (const Category &category : tree) {
        const QString indent(category.depth * 3, QLatin1Char(' '));
        m_category->addItem(indent + category.name, category.id);
    }
    for (int index = 0; index < m_category->count(); ++index) {
        const int id = m_category->itemData(index).toInt();
        for (const Category &category : tree) {
            if (category.id == id && category.parentId < 0
                && category.type == CategoryType::Expense) {
                m_category->setCurrentIndex(index);
                return;
            }
        }
    }
}

void ImportDialog::browse()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Abrir extracto"), QString(), tr("Extractos (*.xlsx *.csv);;Todos (*)"));
    if (!path.isEmpty()) {
        loadFile(path);
    }
}

void ImportDialog::loadFile(const QString &path)
{
    m_path->setText(path);

    QString error;
    QList<ImportedMovement> movements;
    if (!m_service->load(path, &movements, &error)) {
        QMessageBox::warning(this, tr("Importar movimientos"), error);
        return;
    }

    m_movements = movements;
    m_model->setMovements(movements);
    updateSummary();
}

void ImportDialog::updateSummary()
{
    int newCount = 0;
    int duplicateCount = 0;
    int errorCount = 0;
    for (const ImportedMovement &movement : m_model->movements()) {
        if (!movement.error.isEmpty()) {
            ++errorCount;
        } else if (movement.duplicate) {
            ++duplicateCount;
        } else {
            ++newCount;
        }
    }
    m_summary->setText(tr("%1 nuevos \u00b7 %2 duplicados \u00b7 %3 con error \u00b7 %4 seleccionados")
                           .arg(newCount)
                           .arg(duplicateCount)
                           .arg(errorCount)
                           .arg(m_model->selectedIndexes().size()));
    m_importButton->setEnabled(!m_model->selectedIndexes().isEmpty());
}

void ImportDialog::runImport()
{
    const QList<int> selected = m_model->selectedIndexes();
    if (selected.isEmpty()) {
        return;
    }

    const int categoryId = m_category->currentData().toInt();
    ImportResult result;
    QString error;
    if (!m_service->importMovements(m_model->movements(), selected, categoryId, &result, &error)) {
        QMessageBox::critical(this, tr("Importar movimientos"), error);
        return;
    }

    m_imported += result.imported;
    QMessageBox::information(
        this, tr("Importar movimientos"),
        tr("Importadas: %1\nOmitidas: %2\nFallidas: %3")
            .arg(result.imported)
            .arg(result.skipped)
            .arg(result.failed));

    // Recarga el estado (los recien importados ya son duplicados).
    if (!m_path->text().isEmpty()) {
        loadFile(m_path->text());
    }
}
