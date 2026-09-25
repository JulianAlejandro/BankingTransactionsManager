#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QHash>
#include <QList>
#include <QMainWindow>

#include "data/categoryrepository.h"
#include "data/transactionservice.h"
#include "model/derivedtablemodel.h"
#include "model/transactiontablemodel.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(TransactionService *service, CategoryRepository *categoryRepository,
               QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void reload();
    void createTransaction();
    void editSelectedTransaction();
    void deleteSelectedTransaction();
    void updateDerivedDetail();

private:
    void setupToolBar();
    int selectedRow() const;
    int selectedTransactionId() const;

    Ui::MainWindow *ui;
    TransactionService *m_service;
    CategoryRepository *m_categoryRepository;
    TransactionTableModel *m_model;
    DerivedTableModel *m_derivedModel;
    QHash<int, QList<DerivedTransaction>> m_derived;
};

#endif // MAINWINDOW_H
