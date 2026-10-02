#include "ui/reportwindow.h"

#include <QAbstractItemModel>
#include <QAction>
#include <QColor>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLocale>
#include <QMessageBox>
#include <QPainter>
#include <QSplitter>
#include <QStatusBar>
#include <QStringConverter>
#include <QTabWidget>
#include <QTableView>
#include <QTextStream>
#include <QTime>
#include <QToolBar>
#include <QVBoxLayout>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QLegend>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

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

QString shortPath(const QString &path)
{
    const int position = path.lastIndexOf(QStringLiteral(" > "));
    return position < 0 ? path : path.mid(position + 3);
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
    , m_categoryChartView(new QChartView(this))
    , m_evolutionChartView(new QChartView(this))
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

    m_categoryChartView->setRenderHint(QPainter::Antialiasing);
    m_evolutionChartView->setRenderHint(QPainter::Antialiasing);

    QSplitter *categorySplitter = new QSplitter(Qt::Vertical, this);
    categorySplitter->addWidget(m_categoryChartView);
    categorySplitter->addWidget(m_categoryView);

    QSplitter *evolutionSplitter = new QSplitter(Qt::Vertical, this);
    evolutionSplitter->addWidget(m_evolutionChartView);
    evolutionSplitter->addWidget(m_timeView);

    m_tabs->addTab(categorySplitter, tr("Por categoria"));
    m_tabs->addTab(evolutionSplitter, tr("Evolucion"));
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

    QChart *oldCategoryChart = m_categoryChartView->chart();
    m_categoryChartView->setChart(
        buildCategoryChart(categories, m_filters->includeDescendants()));
    delete oldCategoryChart;

    QChart *oldEvolutionChart = m_evolutionChartView->chart();
    m_evolutionChartView->setChart(buildEvolutionChart(buckets));
    delete oldEvolutionChart;

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

QChart *ReportWindow::buildEvolutionChart(const QList<TimeBucket> &buckets) const
{
    QChart *chart = new QChart();
    chart->setTitle(tr("Evolucion"));
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    QLineSeries *income = new QLineSeries();
    income->setName(tr("Ingresos"));
    QLineSeries *expense = new QLineSeries();
    expense->setName(tr("Gastos"));
    QLineSeries *net = new QLineSeries();
    net->setName(tr("Neto"));
    QLineSeries *cumulative = new QLineSeries();
    cumulative->setName(tr("Acumulado"));

    bool first = true;
    double minValue = 0.0;
    double maxValue = 0.0;
    const auto track = [&](double value) {
        if (first) {
            minValue = value;
            maxValue = value;
            first = false;
        } else {
            minValue = qMin(minValue, value);
            maxValue = qMax(maxValue, value);
        }
    };

    for (const TimeBucket &bucket : buckets) {
        const qreal x = QDateTime(bucket.start, QTime(0, 0)).toMSecsSinceEpoch();
        const double incomeValue = bucket.incomeCents / 100.0;
        const double expenseValue = -bucket.expenseCents / 100.0;
        const double netValue = bucket.netCents / 100.0;
        const double cumulativeValue = bucket.cumulativeNetCents / 100.0;
        income->append(x, incomeValue);
        expense->append(x, expenseValue);
        net->append(x, netValue);
        cumulative->append(x, cumulativeValue);
        track(incomeValue);
        track(expenseValue);
        track(netValue);
        track(cumulativeValue);
    }

    chart->addSeries(income);
    chart->addSeries(expense);
    chart->addSeries(net);
    chart->addSeries(cumulative);

    QDateTimeAxis *axisX = new QDateTimeAxis();
    QString format;
    switch (m_filters->granularity()) {
    case Granularity::Day:
    case Granularity::Week:
        format = QStringLiteral("dd/MM");
        break;
    case Granularity::Month:
    case Granularity::Quarter:
        format = QStringLiteral("MM/yy");
        break;
    case Granularity::Year:
        format = QStringLiteral("yyyy");
        break;
    }
    axisX->setFormat(format);
    if (!buckets.isEmpty()) {
        const QDate firstDate = buckets.first().start;
        QDate afterLast = buckets.last().start;
        switch (m_filters->granularity()) {
        case Granularity::Day: afterLast = afterLast.addDays(1); break;
        case Granularity::Week: afterLast = afterLast.addDays(7); break;
        case Granularity::Month: afterLast = afterLast.addMonths(1); break;
        case Granularity::Quarter: afterLast = afterLast.addMonths(3); break;
        case Granularity::Year: afterLast = afterLast.addYears(1); break;
        }
        axisX->setRange(QDateTime(firstDate, QTime(0, 0)),
                        QDateTime(afterLast, QTime(0, 0)));
        axisX->setTickCount(qBound(2, buckets.size(), 12));
    }
    chart->addAxis(axisX, Qt::AlignBottom);

    QValueAxis *axisY = new QValueAxis();
    axisY->setLabelFormat(QStringLiteral("%.0f"));
    double span = maxValue - minValue;
    if (span <= 0.0) {
        span = qMax(1.0, qAbs(maxValue));
    }
    axisY->setRange(minValue - span * 0.1, maxValue + span * 0.1);
    chart->addAxis(axisY, Qt::AlignLeft);

    const QList<QLineSeries *> series = {income, expense, net, cumulative};
    for (QLineSeries *line : series) {
        line->attachAxis(axisX);
        line->attachAxis(axisY);
    }

    return chart;
}

QChart *ReportWindow::buildCategoryChart(const QList<CategoryTotal> &totals,
                                         bool includeDescendants) const
{
    QChart *chart = new QChart();
    chart->setTitle(tr("Importe por categoria"));
    chart->legend()->setVisible(false);

    QBarSeries *series = new QBarSeries();
    QBarSet *set = new QBarSet(tr("Importe"));
    QStringList labels;
    double maxValue = 0.0;

    for (int index = 1; index < totals.size(); ++index) {
        const CategoryTotal &total = totals.at(index);
        const qint64 cents = includeDescendants ? total.subtreeCents : total.directCents;
        const double value = qAbs(cents / 100.0);
        *set << value;
        labels << shortPath(total.path);
        maxValue = qMax(maxValue, value);
    }

    set->setColor(!totals.isEmpty() && totals.first().type == CategoryType::Income
                      ? QColor(0x1b, 0x7f, 0x3b)
                      : QColor(0xb3, 0x26, 0x1e));
    series->append(set);
    chart->addSeries(series);

    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(labels);
    axisX->setLabelsAngle(-45);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setLabelFormat(QStringLiteral("%.0f"));
    axisY->setRange(0.0, maxValue > 0.0 ? maxValue * 1.1 : 1.0);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    return chart;
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
