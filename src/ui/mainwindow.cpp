#include "ui/mainwindow.h"
#include "ui_mainwindow.h"

#include <QAction>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLocale>
#include <QMessageBox>
#include <QStatusBar>
#include <QToolBar>

#include "appinfo.h"
#include "ui/transactioneditordialog.h"

namespace {

QString formatCents(qint64 cents)
{
    const QLocale locale(QLocale::Spanish, QLocale::Spain);
    return locale.toCurrencyString(static_cast<double>(cents) / 100.0,
                                   QStringLiteral("\u20ac"));
}

} // namespace

MainWindow::MainWindow(TransactionService *service, CategoryRepository *categoryRepository,
                       QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_service(service)
    , m_categoryRepository(categoryRepository)
    , m_model(new TransactionTableModel(this))
    , m_derivedModel(new DerivedTableModel(this))
{
    ui->setupUi(this);
    setWindowTitle(AppInfo::DisplayName);

    ui->tableView->setModel(m_model);
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableView->setAlternatingRowColors(false);
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableView->verticalHeader()->setVisible(false);
    ui->tableView->horizontalHeader()->setStretchLastSection(true);

    ui->tableViewDerived->setModel(m_derivedModel);
    ui->tableViewDerived->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableViewDerived->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableViewDerived->setAlternatingRowColors(false);
    ui->tableViewDerived->verticalHeader()->setVisible(false);
    ui->tableViewDerived->horizontalHeader()->setStretchLastSection(true);

    connect(ui->tableView, &QTableView::doubleClicked,
            this, &MainWindow::editSelectedTransaction);
    connect(ui->tableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::updateDerivedDetail);

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
    const int previousId = selectedTransactionId();

    QString error;
    const QList<Category> categories = m_categoryRepository->withPaths(&error);
    if (!error.isEmpty()) {
        QMessageBox::warning(this, AppInfo::DisplayName, error);
        error.clear();
    }

    const QList<Transaction> transactions = m_service->all(&error);
    if (!error.isEmpty()) {
        QMessageBox::warning(this, AppInfo::DisplayName, error);
        error.clear();
    }

    const QHash<int, QList<DerivedTransaction>> derived = m_service->allDerived(&error);
    if (!error.isEmpty()) {
        QMessageBox::warning(this, AppInfo::DisplayName, error);
    }

    m_derived = derived;
    m_derivedModel->setCategories(categories);
    m_model->setTransactions(transactions, derived, categories);

    int rowToSelect = -1;
    if (previousId >= 0) {
        for (int row = 0; row < transactions.size(); ++row) {
            if (transactions.at(row).id == previousId) {
                rowToSelect = row;
                break;
            }
        }
    }
    if (rowToSelect < 0 && !transactions.isEmpty()) {
        rowToSelect = 0;
    }

    if (rowToSelect >= 0) {
        ui->tableView->selectRow(rowToSelect);
    } else {
        updateDerivedDetail();
    }

    statusBar()->showMessage(tr("%1 transacciones").arg(transactions.size()));
}

int MainWindow::selectedRow() const
{
    const QModelIndexList selection = ui->tableView->selectionModel()->selectedRows();
    return selection.isEmpty() ? -1 : selection.first().row();
}

int MainWindow::selectedTransactionId() const
{
    const int row = selectedRow();
    Transaction transaction;
    if (row < 0 || !m_model->transactionAt(row, &transaction)) {
        return -1;
    }
    return transaction.id;
}

void MainWindow::updateDerivedDetail()
{
    const int row = selectedRow();
    Transaction transaction;
    if (row < 0 || !m_model->transactionAt(row, &transaction)) {
        m_derivedModel->setRows(QList<DerivedTransaction>());
        ui->labelDerivedSummary->setText(
            tr("Selecciona una transaccion para ver su desglose"));
        return;
    }

    const QList<DerivedTransaction> derived = m_derived.value(transaction.id);
    m_derivedModel->setRows(derived);

    qint64 total = 0;
    for (const DerivedTransaction &item : derived) {
        total += item.amountCents;
    }

    ui->labelDerivedSummary->setText(tr("%1  \u00b7  %2 derivada(s)  \u00b7  total %3")
                                         .arg(transaction.concept)
                                         .arg(derived.size())
                                         .arg(formatCents(total)));
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

    TransactionEditorDialog dialog(categories, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    Transaction transaction = dialog.transaction();
    QList<DerivedTransaction> derived = dialog.derivedTransactions();
    if (!m_service->create(transaction, derived, nullptr, &error)) {
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
    QList<DerivedTransaction> derived;
    if (!m_service->load(transaction.id, &transaction, &derived, &error)) {
        QMessageBox::critical(this, AppInfo::DisplayName, error);
        return;
    }

    const QList<Category> categories = m_categoryRepository->withPaths(&error);
    TransactionEditorDialog dialog(categories, this);
    dialog.setTransaction(transaction, derived);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    transaction = dialog.transaction();
    derived = dialog.derivedTransactions();
    if (!m_service->save(transaction, derived, &error)) {
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
        tr("\u00bfEliminar la transaccion \"%1\" y su desglose?").arg(transaction.concept));
    if (answer != QMessageBox::Yes) {
        return;
    }

    QString error;
    if (!m_service->remove(transaction.id, &error)) {
        QMessageBox::critical(this, AppInfo::DisplayName, error);
        return;
    }
    reload();
}
