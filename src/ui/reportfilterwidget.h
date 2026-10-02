#ifndef REPORTFILTERWIDGET_H
#define REPORTFILTERWIDGET_H

#include <QWidget>

#include "report/reporttypes.h"

class QCheckBox;
class QComboBox;
class QDateEdit;
class CategoryRepository;

class ReportFilterWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ReportFilterWidget(CategoryRepository *categories, QWidget *parent = nullptr);

    ReportPeriod period() const;
    int rootCategoryId() const;
    bool includeDescendants() const;
    Granularity granularity() const;
    bool hasComparison() const;
    ReportPeriod comparisonPeriod() const;

signals:
    void changed();

private slots:
    void applyPreset();
    void applyCompareState();
    void emitChanged();

private:
    void populateCategories(CategoryRepository *categories);

    QComboBox *m_preset;
    QDateEdit *m_from;
    QDateEdit *m_to;
    QComboBox *m_category;
    QCheckBox *m_descendants;
    QComboBox *m_granularity;
    QComboBox *m_compare;
    QDateEdit *m_compareFrom;
    QDateEdit *m_compareTo;
};

#endif // REPORTFILTERWIDGET_H
