#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "data/categoryrepository.h"
#include "data/transactionrepository.h"
#include "model/transactiontablemodel.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(TransactionRepository *transactionRepository,
               CategoryRepository *categoryRepository,
               QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void reload();
    void createTransaction();
    void editSelectedTransaction();
    void deleteSelectedTransaction();

private:
    void setupToolBar();
    int selectedRow() const;

    Ui::MainWindow *ui;
    TransactionRepository *m_transactionRepository;
    CategoryRepository *m_categoryRepository;
    TransactionTableModel *m_model;
};

#endif // MAINWINDOW_H
