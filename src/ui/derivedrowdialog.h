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

class DerivedRowDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DerivedRowDialog(const QList<Category> &categories, QWidget *parent = nullptr);
    ~DerivedRowDialog() override;

    void setRow(const DerivedTransaction &row);
    void setSuggestedMagnitudeCents(qint64 cents);
    DerivedTransaction row() const;

private slots:
    void updateSignHint();
    void validateAndAccept();

private:
    void populateCategories(const QList<Category> &categories);

    Ui::DerivedRowDialog *ui;
    DerivedTransaction m_row;
    QHash<int, CategoryType> m_typeById;
};

#endif // DERIVEDROWDIALOG_H
