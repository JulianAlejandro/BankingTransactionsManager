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
    void setParentInfo(const QString &concept, qint64 amountCents, const QString &notes);
    DerivedTransaction row() const;

private slots:
    void loadFromParent();
    void updateSignHint();
    void validateAndAccept();

private:
    void populateCategories(const QList<Category> &categories);

    Ui::DerivedRowDialog *ui;
    DerivedTransaction m_row;
    QHash<int, CategoryType> m_typeById;

    bool m_hasParent = false;
    QString m_parentConcept;
    qint64 m_parentAmountCents = 0;
    QString m_parentNotes;
};

#endif // DERIVEDROWDIALOG_H
