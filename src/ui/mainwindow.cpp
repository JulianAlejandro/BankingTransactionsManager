#include "ui/mainwindow.h"
#include "ui_mainwindow.h"

#include <QAction>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QMessageBox>
#include <QStatusBar>
#include <QToolBar>

#include "appinfo.h"
#include "ui/transactiondialog.h"

MainWindow::MainWindow(TransactionRepository *transactionRepository,
                       CategoryRepository *categoryRepository,
                       QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_transactionRepository(transactionRepository)
    , m_categoryRepository(categoryRepository)
    , m_model(new TransactionTableModel(this))
{
    ui->setupUi(this);
    setWindowTitle(AppInfo::DisplayName);

    ui->tableView->setModel(m_model);
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableView->setAlternatingRowColors(true);
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableView->verticalHeader()->setVisible(false);
    ui->tableView->horizontalHeader()->setStretchLastSection(true);

    setupToolBar();
    reload();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupToolBar()
{
    QToolBar *toolBar = addToolBar(tr("Principal"));
    toolBar->setMovable(false);

    QAction *newAction = toolBar->addAction(tr("Nuevo"));
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &MainWindow::createTransaction);

    QAction *editAction = toolBar->addAction(tr("Editar"));
    connect(editAction, &QAction::triggered, this, &MainWindow::editSelectedTransaction);

    QAction *deleteAction = toolBar->addAction(tr("Eliminar"));
    deleteAction->setShortcut(QKeySequence::Delete);
    connect(deleteAction, &QAction::triggered, this, &MainWindow::deleteSelectedTransaction);

    toolBar->addSeparator();

    QAction *refreshAction = toolBar->addAction(tr("Refrescar"));
    refreshAction->setShortcut(QKeySequence::Refresh);
    connect(refreshAction, &QAction::triggered, this, &MainWindow::reload);
}

void MainWindow::reload()
{
    QString error;
    const QList<Category> categories = m_categoryRepository->withPaths(&error);
    if (!error.isEmpty()) {
        QMessageBox::warning(this, AppInfo::DisplayName, error);
        error.clear();
    }

    const QList<Transaction> transactions = m_transactionRepository->all(&error);
    if (!error.isEmpty()) {
        QMessageBox::warning(this, AppInfo::DisplayName, error);
    }

    m_model->setTransactions(transactions, categories);
    statusBar()->showMessage(tr("%1 transacciones").arg(transactions.size()));
}

int MainWindow::selectedRow() const
{
    const QModelIndexList selection = ui->tableView->selectionModel()->selectedRows();
    return selection.isEmpty() ? -1 : selection.first().row();
}

void MainWindow::createTransaction()
{
    QString error;
    const QList<Category> categories = m_categoryRepository->withPaths(&error);
    if (categories.isEmpty()) {
        QMessageBox::warning(this, AppInfo::DisplayName,
                             tr("No hay categorias disponibles."));
        return;
    }

    TransactionDialog dialog(categories, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    Transaction transaction = dialog.transaction();
    if (!m_transactionRepository->add(transaction, &error)) {
        QMessageBox::critical(this, AppInfo::DisplayName, error);
        return;
    }
    reload();
}

void MainWindow::editSelectedTransaction()
{
    const int row = selectedRow();
    Transaction transaction;
    if (row < 0 || !m_model->transactionAt(row, &transaction)) {
        QMessageBox::information(this, AppInfo::DisplayName,
                                 tr("Selecciona una transaccion."));
        return;
    }

    QString error;
    const QList<Category> categories = m_categoryRepository->withPaths(&error);

    TransactionDialog dialog(categories, this);
    dialog.setTransaction(transaction);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    transaction = dialog.transaction();
    if (!m_transactionRepository->update(transaction, &error)) {
        QMessageBox::critical(this, AppInfo::DisplayName, error);
        return;
    }
    reload();
}

void MainWindow::deleteSelectedTransaction()
{
    const int row = selectedRow();
    Transaction transaction;
    if (row < 0 || !m_model->transactionAt(row, &transaction)) {
        QMessageBox::information(this, AppInfo::DisplayName,
                                 tr("Selecciona una transaccion."));
        return;
    }

    const QMessageBox::StandardButton answer = QMessageBox::question(
        this, AppInfo::DisplayName,
        tr("\u00bfEliminar la transaccion \"%1\"?").arg(transaction.concept));
    if (answer != QMessageBox::Yes) {
        return;
    }

    QString error;
    if (!m_transactionRepository->remove(transaction.id, &error)) {
        QMessageBox::critical(this, AppInfo::DisplayName, error);
        return;
    }
    reload();
}
