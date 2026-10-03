#ifndef DERIVEDROWDIALOG_H
#define DERIVEDROWDIALOG_H

#include <QDialog>
#include <QHash>
#include <QList>

#include "model/category.h"
#include "model/derivedtransaction.h"

QT_BEGIN_NAMESPACE
namespace Ui { class DerivedRowDialog; }
QT_END_NAMESPACE

class CategoryRepository;

class DerivedRowDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DerivedRowDialog(CategoryRepository *repository, QWidget *parent = nullptr);
    ~DerivedRowDialog() override;

    void setRow(const DerivedTransaction &row);
    void setSuggestedMagnitudeCents(qint64 cents);
    void setParentInfo(const QString &concept, qint64 amountCents, const QString &notes);
    DerivedTransaction row() const;

private slots:
    void loadFromParent();
    void updateSignHint();
    void createCategory();
    void validateAndAccept();

private:
    void reloadCategories(int selectId = -1);
    Category currentCategory() const;

    Ui::DerivedRowDialog *ui;
    CategoryRepository *m_repository;
    DerivedTransaction m_row;
    QHash<int, CategoryType> m_typeById;

    bool m_hasParent = false;
    QString m_parentConcept;
    qint64 m_parentAmountCents = 0;
    QString m_parentNotes;
};

#endif // DERIVEDROWDIALOG_H
