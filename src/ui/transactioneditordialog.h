#ifndef TRANSACTIONEDITORDIALOG_H
#define TRANSACTIONEDITORDIALOG_H

#include <QDialog>
#include <QList>

#include "model/category.h"
#include "model/derivedtransaction.h"
#include "model/derivedtablemodel.h"
#include "model/transaction.h"

QT_BEGIN_NAMESPACE
namespace Ui { class TransactionEditorDialog; }
QT_END_NAMESPACE

class TransactionEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TransactionEditorDialog(const QList<Category> &categories,
                                     QWidget *parent = nullptr);
    ~TransactionEditorDialog() override;

    void setTransaction(const Transaction &transaction,
                        const QList<DerivedTransaction> &derived);
    Transaction transaction() const;
    QList<DerivedTransaction> derivedTransactions() const;

private slots:
    void addDerived();
    void editDerived();
    void removeDerived();
    void refreshState();
    void validateAndAccept();

private:
    qint64 amountCents() const;
    int selectedRow() const;
    bool validateState(QString *error) const;

    Ui::TransactionEditorDialog *ui;
    DerivedTableModel *m_model;
    QList<Category> m_categories;
    Transaction m_transaction;
};

#endif // TRANSACTIONEDITORDIALOG_H
