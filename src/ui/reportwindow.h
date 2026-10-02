#ifndef REPORTWINDOW_H
#define REPORTWINDOW_H

#include <QMainWindow>
#include <QModelIndex>

#include "report/reporttypes.h"

class QAbstractItemModel;
class QChart;
class QChartView;
class QLabel;
class QTabWidget;
class QTableView;
class CategoryRepository;
class ComparisonTableModel;
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
    void showCategoryMovements(const QModelIndex &index);

private:
    void updateTotals(const PeriodTotals &totals);
    QChart *buildEvolutionChart(const QList<TimeBucket> &buckets,
                                bool *hasData = nullptr) const;
    QChart *buildCategoryChart(const QList<CategoryTotal> &totals,
                               bool includeDescendants, bool *hasData = nullptr) const;
    QChart *buildComparisonChart(const PeriodTotals &a, const PeriodTotals &b,
                                 bool *hasData = nullptr) const;
    bool writeModelCsv(const QString &path, QAbstractItemModel *model) const;

    ReportRepository *m_reports;
    CategoryRepository *m_categories;
    ReportFilterWidget *m_filters;
    ReportTableModel *m_categoryModel;
    TimeSeriesModel *m_timeModel;
    ComparisonTableModel *m_compareModel;
    QTableView *m_categoryView;
    QTableView *m_timeView;
    QTableView *m_compareView;
    QChartView *m_categoryChartView;
    QChartView *m_evolutionChartView;
    QChartView *m_compareChartView;
    QLabel *m_compareSummary;
    QTabWidget *m_tabs;
    QLabel *m_incomeValue;
    QLabel *m_expenseValue;
    QLabel *m_netValue;
    QLabel *m_countValue;
};

#endif // REPORTWINDOW_H
