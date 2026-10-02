#include "ui/reportwindow.h"

#include <QAbstractItemModel>
#include <QAction>
#include <QFile>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLocale>
#include <QMessageBox>
#include <QStatusBar>
#include <QStringConverter>
#include <QTabWidget>
#include <QTableView>
#include <QTextStream>
#include <QToolBar>
#include <QVBoxLayout>

#include "data/categoryrepository.h"
#include "data/reportrepository.h"
#include "model/reporttablemodel.h"
#include "model/timeseriesmodel.h"
#include "ui/reportfilterwidget.h"

namespace {

QLocale esLocale()
{
    return QLocale(QLocale::Spanish, QLocale::Spain);
}

QString formattedAmount(qint64 cents)
{
    return esLocale().toCurrencyString(static_cast<double>(cents) / 100.0,
                                       QStringLiteral("\u20ac"));
}

QGroupBox *createKpi(const QString &title, QLabel **valueLabel, QWidget *parent)
{
    QGroupBox *box = new QGroupBox(title, parent);
    QVBoxLayout *layout = new QVBoxLayout(box);
    QLabel *value = new QLabel(QStringLiteral("\u2014"), box);
    QFont font = value->font();
    font.setPointSize(font.pointSize() + 6);
    font.setBold(true);
    value->setFont(font);
    value->setAlignment(Qt::AlignCenter);
    layout->addWidget(value);
    *valueLabel = value;
    return box;
}

QString csvField(const QString &value)
{
    if (value.contains(QLatin1Char(';')) || value.contains(QLatin1Char('"'))
        || value.contains(QLatin1Char('\n'))) {
        QString escaped = value;
        escaped.replace(QLatin1Char('"'), QStringLiteral("\"\""));
        return QLatin1Char('"') + escaped + QLatin1Char('"');
    }
    return value;
}

} // namespace

ReportWindow::ReportWindow(ReportRepository *reports, CategoryRepository *categories,
                           QWidget *parent)
    : QMainWindow(parent)
    , m_reports(reports)
    , m_categories(categories)
    , m_filters(new ReportFilterWidget(categories, this))
    , m_categoryModel(new ReportTableModel(this))
    , m_timeModel(new TimeSeriesModel(this))
    , m_categoryView(new QTableView(this))
    , m_timeView(new QTableView(this))
    , m_tabs(new QTabWidget(this))
{
    setWindowTitle(tr("Informes"));

    QWidget *central = new QWidget(this);
    QVBoxLayout *root = new QVBoxLayout(central);
    root->addWidget(m_filters);

    QHBoxLayout *kpiLayout = new QHBoxLayout();
    kpiLayout->addWidget(createKpi(tr("Ingresos"), &m_incomeValue, central));
    kpiLayout->addWidget(createKpi(tr("Gastos"), &m_expenseValue, central));
    kpiLayout->addWidget(createKpi(tr("Neto"), &m_netValue, central));
    kpiLayout->addWidget(createKpi(tr("Movimientos"), &m_countValue, central));
    root->addLayout(kpiLayout);

    m_categoryView->setModel(m_categoryModel);
    m_categoryView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_categoryView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_categoryView->setAlternatingRowColors(false);
    m_categoryView->verticalHeader()->setVisible(false);
    m_categoryView->horizontalHeader()->setSectionResizeMode(
        ReportTableModel::CategoryColumn, QHeaderView::Stretch);
    m_categoryView->horizontalHeader()->setStretchLastSection(true);

    m_timeView->setModel(m_timeModel);
    m_timeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_timeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_timeView->setAlternatingRowColors(false);
    m_timeView->verticalHeader()->setVisible(false);
    m_timeView->horizontalHeader()->setSectionResizeMode(
        TimeSeriesModel::PeriodColumn, QHeaderView::Stretch);
    m_timeView->horizontalHeader()->setStretchLastSection(true);

    m_tabs->addTab(m_categoryView, tr("Por categoria"));
    m_tabs->addTab(m_timeView, tr("Evolucion"));
    root->addWidget(m_tabs, 1);

    setCentralWidget(central);

    QToolBar *toolBar = addToolBar(tr("Informes"));
    toolBar->setMovable(false);
    QAction *refreshAction = toolBar->addAction(tr("Actualizar"));
    connect(refreshAction, &QAction::triggered, this, &ReportWindow::refresh);
    QAction *exportAction = toolBar->addAction(tr("Exportar CSV"));
    connect(exportAction, &QAction::triggered, this, &ReportWindow::exportCsv);

    connect(m_filters, &ReportFilterWidget::changed, this, &ReportWindow::refresh);

    resize(900, 620);
    refresh();
}

void ReportWindow::refresh()
{
    const ReportPeriod period = m_filters->period();
    QString error;

    PeriodTotals totals;
    if (!m_reports->totals(period, &totals, &error)) {
        QMessageBox::warning(this, tr("Informes"), error);
        return;
    }
    updateTotals(totals);

    QList<CategoryTotal> categories;
    const int rootId = m_filters->rootCategoryId();
    if (rootId >= 0) {
        if (!m_reports->categoryBreakdown(period, rootId, &categories, &error)) {
            QMessageBox::warning(this, tr("Informes"), error);
        }
    }
    m_categoryModel->setTotals(categories, m_filters->includeDescendants());

    QList<TimeBucket> buckets;
    if (!m_reports->timeSeries(period, m_filters->granularity(), &buckets, &error)) {
        QMessageBox::warning(this, tr("Informes"), error);
    }
    m_timeModel->setBuckets(buckets, m_filters->granularity());

    statusBar()->showMessage(tr("%1 movimientos en el periodo").arg(totals.count));
}

void ReportWindow::updateTotals(const PeriodTotals &totals)
{
    m_incomeValue->setText(formattedAmount(totals.incomeCents));
    m_expenseValue->setText(formattedAmount(-totals.expenseCents));
    m_netValue->setText(formattedAmount(totals.netCents));
    m_countValue->setText(QString::number(totals.count));
    m_expenseValue->setStyleSheet(QStringLiteral("color: #b3261e"));
    m_incomeValue->setStyleSheet(QStringLiteral("color: #1b7f3b"));
    m_netValue->setStyleSheet(totals.netCents >= 0 ? QStringLiteral("color: #1b7f3b")
                                                   : QStringLiteral("color: #b3261e"));
}

void ReportWindow::exportCsv()
{
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Exportar informe"), QString(), tr("CSV (*.csv)"));
    if (path.isEmpty()) {
        return;
    }

    QAbstractItemModel *model = m_tabs->currentIndex() == 0
        ? static_cast<QAbstractItemModel *>(m_categoryModel)
        : static_cast<QAbstractItemModel *>(m_timeModel);

    if (!writeModelCsv(path, model)) {
        QMessageBox::critical(this, tr("Informes"), tr("No se pudo escribir el fichero."));
        return;
    }
    statusBar()->showMessage(tr("Informe exportado a %1").arg(path));
}

bool ReportWindow::writeModelCsv(const QString &path, QAbstractItemModel *model) const
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << QChar(0xFEFF);

    QStringList headers;
    for (int column = 0; column < model->columnCount(); ++column) {
        headers << csvField(model->headerData(column, Qt::Horizontal).toString());
    }
    out << headers.join(QLatin1Char(';')) << QLatin1Char('\n');

    for (int row = 0; row < model->rowCount(); ++row) {
        QStringList cells;
        for (int column = 0; column < model->columnCount(); ++column) {
            cells << csvField(
                model->index(row, column).data(Qt::UserRole + 1).toString());
        }
        out << cells.join(QLatin1Char(';')) << QLatin1Char('\n');
    }

    return true;
}
