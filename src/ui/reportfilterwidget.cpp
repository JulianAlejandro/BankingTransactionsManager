#include "ui/reportfilterwidget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QGridLayout>
#include <QLabel>

#include "data/categoryrepository.h"

ReportFilterWidget::ReportFilterWidget(CategoryRepository *categories, QWidget *parent)
    : QWidget(parent)
    , m_preset(new QComboBox(this))
    , m_from(new QDateEdit(this))
    , m_to(new QDateEdit(this))
    , m_category(new QComboBox(this))
    , m_descendants(new QCheckBox(tr("Incluir descendientes"), this))
    , m_granularity(new QComboBox(this))
{
    m_preset->addItems({tr("Personalizado"), tr("Este mes"), tr("Mes anterior"),
                        tr("Este trimestre"), tr("Este ano"), tr("Todo")});

    m_from->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    m_to->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    m_from->setCalendarPopup(true);
    m_to->setCalendarPopup(true);

    m_descendants->setChecked(true);

    m_granularity->addItem(tr("Dia"), static_cast<int>(Granularity::Day));
    m_granularity->addItem(tr("Semana"), static_cast<int>(Granularity::Week));
    m_granularity->addItem(tr("Mes"), static_cast<int>(Granularity::Month));
    m_granularity->addItem(tr("Trimestre"), static_cast<int>(Granularity::Quarter));
    m_granularity->addItem(tr("Ano"), static_cast<int>(Granularity::Year));
    m_granularity->setCurrentIndex(2);

    populateCategories(categories);

    QGridLayout *layout = new QGridLayout(this);
    layout->addWidget(new QLabel(tr("Periodo"), this), 0, 0);
    layout->addWidget(m_preset, 0, 1);
    layout->addWidget(new QLabel(tr("Desde"), this), 0, 2);
    layout->addWidget(m_from, 0, 3);
    layout->addWidget(new QLabel(tr("Hasta"), this), 0, 4);
    layout->addWidget(m_to, 0, 5);
    layout->addWidget(new QLabel(tr("Categoria"), this), 1, 0);
    layout->addWidget(m_category, 1, 1, 1, 2);
    layout->addWidget(m_descendants, 1, 3);
    layout->addWidget(new QLabel(tr("Granularidad"), this), 1, 4);
    layout->addWidget(m_granularity, 1, 5);
    layout->setColumnStretch(1, 1);

    connect(m_preset, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { applyPreset(); });
    connect(m_category, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ReportFilterWidget::emitChanged);
    connect(m_descendants, &QCheckBox::toggled, this, &ReportFilterWidget::emitChanged);
    connect(m_granularity, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ReportFilterWidget::emitChanged);
    connect(m_from, &QDateEdit::dateChanged, this, &ReportFilterWidget::emitChanged);
    connect(m_to, &QDateEdit::dateChanged, this, &ReportFilterWidget::emitChanged);

    applyPreset();
}

void ReportFilterWidget::populateCategories(CategoryRepository *categories)
{
    if (categories == nullptr) {
        return;
    }

    QString error;
    const QList<Category> tree = categories->withPaths(&error);
    for (const Category &category : tree) {
        const QString indent(category.depth * 3, QLatin1Char(' '));
        m_category->addItem(indent + category.name, category.id);
    }

    int defaultIndex = -1;
    for (int index = 0; index < m_category->count(); ++index) {
        const int id = m_category->itemData(index).toInt();
        for (const Category &category : tree) {
            if (category.id == id && category.parentId < 0
                && category.type == CategoryType::Expense) {
                defaultIndex = index;
                break;
            }
        }
        if (defaultIndex >= 0) {
            break;
        }
    }
    if (defaultIndex >= 0) {
        m_category->setCurrentIndex(defaultIndex);
    }
}

void ReportFilterWidget::applyPreset()
{
    const int index = m_preset->currentIndex();
    const bool custom = index == 0;
    const QDate today = QDate::currentDate();

    if (!custom) {
        QDate from;
        QDate to;
        switch (index) {
        case 1: // Este mes
            from = QDate(today.year(), today.month(), 1);
            to = from.addMonths(1).addDays(-1);
            break;
        case 2: // Mes anterior
            from = QDate(today.year(), today.month(), 1).addMonths(-1);
            to = from.addMonths(1).addDays(-1);
            break;
        case 3: // Este trimestre
            from = QDate(today.year(), ((today.month() - 1) / 3) * 3 + 1, 1);
            to = from.addMonths(3).addDays(-1);
            break;
        case 4: // Este ano
            from = QDate(today.year(), 1, 1);
            to = QDate(today.year(), 12, 31);
            break;
        case 5: // Todo
            from = QDate(2000, 1, 1);
            to = today;
            break;
        default:
            break;
        }

        if (from.isValid() && to.isValid()) {
            m_from->setDate(from);
            m_to->setDate(to);
        }
    }

    m_from->setReadOnly(!custom);
    m_to->setReadOnly(!custom);
    emitChanged();
}

void ReportFilterWidget::emitChanged()
{
    emit changed();
}

ReportPeriod ReportFilterWidget::period() const
{
    return {m_from->date(), m_to->date()};
}

int ReportFilterWidget::rootCategoryId() const
{
    return m_category->currentData().toInt();
}

bool ReportFilterWidget::includeDescendants() const
{
    return m_descendants->isChecked();
}

Granularity ReportFilterWidget::granularity() const
{
    return static_cast<Granularity>(m_granularity->currentData().toInt());
}
