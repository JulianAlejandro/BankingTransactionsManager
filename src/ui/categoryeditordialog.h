#ifndef CATEGORYEDITORDIALOG_H
#define CATEGORYEDITORDIALOG_H

#include <QDialog>

#include "model/category.h"

class QLineEdit;
class QLabel;
class CategoryRepository;

// Alta de una categoria hija de una categoría padre ya existente. El tipo se
// hereda del padre, por lo que no es editable.
class CategoryEditorDialog : public QDialog
{
    Q_OBJECT

public:
    CategoryEditorDialog(CategoryRepository *repository, const Category &parent,
                         QWidget *dialogParent = nullptr);

    QString name() const;
    Category createdCategory() const;

private slots:
    void validateAndAccept();

private:
    CategoryRepository *m_repository;
    Category m_parent;
    Category m_created;

    QLineEdit *m_name;
};

#endif // CATEGORYEDITORDIALOG_H
