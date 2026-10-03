#include "ui/categoryeditordialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QVBoxLayout>

#include "data/categoryrepository.h"

CategoryEditorDialog::CategoryEditorDialog(CategoryRepository *repository,
                                           const Category &parent, QWidget *dialogParent)
    : QDialog(dialogParent)
    , m_repository(repository)
    , m_parent(parent)
    , m_name(new QLineEdit(this))
{
    setWindowTitle(tr("Nueva categoria"));

    const QString parentPath = parent.path.isEmpty() ? parent.name : parent.path;

    QFormLayout *form = new QFormLayout();
    form->addRow(tr("Categoria padre"), new QLabel(parentPath, this));
    form->addRow(tr("Nombre"), m_name);
    form->addRow(tr("Tipo"),
                 new QLabel(parent.type == CategoryType::Income ? tr("Ingreso") : tr("Gasto"),
                            this));

    QDialogButtonBox *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &CategoryEditorDialog::validateAndAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);

    m_name->setFocus();
}

QString CategoryEditorDialog::name() const
{
    return m_name->text().trimmed();
}

Category CategoryEditorDialog::createdCategory() const
{
    return m_created;
}

void CategoryEditorDialog::validateAndAccept()
{
    const QString candidate = name();
    if (candidate.isEmpty()) {
        QMessageBox::warning(this, tr("Datos incompletos"),
                             tr("El nombre no puede estar vacio."));
        return;
    }
    if (m_repository == nullptr) {
        QMessageBox::critical(this, tr("Nueva categoria"),
                              tr("No hay repositorio de categorias."));
        return;
    }

    Category created;
    QString error;
    if (!m_repository->add(m_parent.id, candidate, m_parent.type, &created, &error)) {
        QMessageBox::critical(this, tr("Nueva categoria"), error);
        return;
    }

    m_created = created;
    accept();
}
