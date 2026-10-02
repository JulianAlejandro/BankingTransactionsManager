#ifndef REPORTWINDOW_H
#define REPORTWINDOW_H

#include <QMainWindow>

#include "report/reporttypes.h"

class QAbstractItemModel;
class QChart;
class QChartView;
class QLabel;
class QTabWidget;
class QTableView;
class CategoryRepository;
class ReportFilterWidget;
class ReportRepository;
class ReportTableModel;
class TimeSeriesModel;

class ReportWindow : public QMainWindow
{
    Q_OBJECT

public:
    ReportWindow(ReportRepository *reports, CategoryRepository *categories,
                 QWidget *parent = nullptr);

public slots:
    void refresh();

private slots:
    void exportCsv();

private:
    void updateTotals(const PeriodTotals &totals);
    QChart *buildEvolutionChart(const QList<TimeBucket> &buckets) const;
    QChart *buildCategoryChart(const QList<CategoryTotal> &totals,
                               bool includeDescendants) const;
    bool writeModelCsv(const QString &path, QAbstractItemModel *model) const;

    ReportRepository *m_reports;
    CategoryRepository *m_categories;
    ReportFilterWidget *m_filters;
    ReportTableModel *m_categoryModel;
    TimeSeriesModel *m_timeModel;
    QTableView *m_categoryView;
    QTableView *m_timeView;
    QChartView *m_categoryChartView;
    QChartView *m_evolutionChartView;
    QTabWidget *m_tabs;
    QLabel *m_incomeValue;
    QLabel *m_expenseValue;
    QLabel *m_netValue;
    QLabel *m_countValue;
};

#endif // REPORTWINDOW_H
