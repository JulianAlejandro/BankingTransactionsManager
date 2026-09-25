#ifndef TRANSACTIONDIALOG_H
#define TRANSACTIONDIALOG_H

#include <QDialog>
#include <QList>

#include "model/category.h"
#include "model/transaction.h"

QT_BEGIN_NAMESPACE
namespace Ui { class TransactionDialog; }
QT_END_NAMESPACE

class TransactionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TransactionDialog(const QList<Category> &categories, QWidget *parent = nullptr);
    ~TransactionDialog() override;

    void setTransaction(const Transaction &transaction);
    Transaction transaction() const;

private slots:
    void validateAndAccept();

private:
    void populateCategories(const QList<Category> &categories);

    Ui::TransactionDialog *ui;
    Transaction m_transaction;
};

#endif // TRANSACTIONDIALOG_H
