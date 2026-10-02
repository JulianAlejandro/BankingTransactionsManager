#include <QtTest>

#include <memory>

#include "data/categoryrepository.h"
#include "data/database.h"
#include "data/reportrepository.h"
#include "data/transactionservice.h"
#include "model/derivedtransaction.h"
#include "model/transaction.h"

class TestReportRepository : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void totalsSumsIncomeExpenseAndCount();
    void totalsOfEmptyPeriodAreZero();
    void periodBoundsAreInclusive();
    void categoryBreakdownDirectAndSubtree();
    void categoryBreakdownRestrictedToRoot();
    void timeSeriesMonthlyBucketsAndCumulative();
    void timeSeriesFillsEmptyBuckets();
    void timeSeriesWeeksStartOnMonday();

private:
    int categoryIdByName(const QString &name) const;
    int addTransaction(const QDate &date, qint64 amountCents, int categoryId,
                       const QString &concept);
    const CategoryTotal *totalById(const QList<CategoryTotal> &totals, int categoryId) const;

    Database m_db;
    std::unique_ptr<CategoryRepository> m_categories;
    std::unique_ptr<TransactionService> m_service;
    std::unique_ptr<ReportRepository> m_reports;
};

void TestReportRepository::init()
{
    QString error;
    QVERIFY2(m_db.open(QStringLiteral(":memory:"), &error), qPrintable(error));
    m_categories = std::make_unique<CategoryRepository>(m_db.connection());
    m_service = std::make_unique<TransactionService>(m_db.connection());
    m_reports = std::make_unique<ReportRepository>(m_db.connection());
}

void TestReportRepository::cleanup()
{
    m_reports.reset();
    m_service.reset();
    m_categories.reset();
    m_db.close();
}

int TestReportRepository::categoryIdByName(const QString &name) const
{
    const QList<Category> categories = m_categories->all();
    for (const Category &category : categories) {
        if (category.name == name) {
            return category.id;
        }
    }
    return -1;
}

int TestReportRepository::addTransaction(const QDate &date, qint64 amountCents, int categoryId,
                                         const QString &concept)
{
    Transaction transaction;
    transaction.date = date;
    transaction.concept = concept;
    transaction.amountCents = amountCents;

    DerivedTransaction derived;
    derived.categoryId = categoryId;
    derived.amountCents = amountCents;
    derived.concept = concept;

    QList<DerivedTransaction> derivedList;
    derivedList << derived;

    QString error;
    if (!m_service->create(transaction, derivedList, nullptr, &error)) {
        return -1;
    }
    return transaction.id;
}

const CategoryTotal *TestReportRepository::totalById(const QList<CategoryTotal> &totals,
                                                     int categoryId) const
{
    for (const CategoryTotal &total : totals) {
        if (total.categoryId == categoryId) {
            return &total;
        }
    }
    return nullptr;
}

void TestReportRepository::totalsSumsIncomeExpenseAndCount()
{
    const int salario = categoryIdByName(QStringLiteral("Salario"));
    const int restaurantes = categoryIdByName(QStringLiteral("Restaurantes"));
    QVERIFY(salario > 0);
    QVERIFY(restaurantes > 0);

    QVERIFY(addTransaction(QDate(2026, 3, 10), 100000, salario, QStringLiteral("Nomina")) > 0);
    QVERIFY(addTransaction(QDate(2026, 3, 15), -30000, restaurantes, QStringLiteral("Cena")) > 0);

    PeriodTotals totals;
    QString error;
    QVERIFY2(m_reports->totals({QDate(2026, 3, 1), QDate(2026, 3, 31)}, &totals, &error),
             qPrintable(error));
    QCOMPARE(totals.incomeCents, qint64(100000));
    QCOMPARE(totals.expenseCents, qint64(-30000));
    QCOMPARE(totals.netCents, qint64(70000));
    QCOMPARE(totals.count, 2);
}

void TestReportRepository::totalsOfEmptyPeriodAreZero()
{
    const int salario = categoryIdByName(QStringLiteral("Salario"));
    QVERIFY(salario > 0);
    QVERIFY(addTransaction(QDate(2026, 1, 5), 50000, salario, QStringLiteral("Nomina")) > 0);

    PeriodTotals totals;
    QString error;
    QVERIFY2(m_reports->totals({QDate(2026, 2, 1), QDate(2026, 2, 28)}, &totals, &error),
             qPrintable(error));
    QCOMPARE(totals.incomeCents, qint64(0));
    QCOMPARE(totals.expenseCents, qint64(0));
    QCOMPARE(totals.netCents, qint64(0));
    QCOMPARE(totals.count, 0);
}

void TestReportRepository::periodBoundsAreInclusive()
{
    const int restaurantes = categoryIdByName(QStringLiteral("Restaurantes"));
    QVERIFY(restaurantes > 0);

    QVERIFY(addTransaction(QDate(2026, 3, 1), -1000, restaurantes, QStringLiteral("Desde")) > 0);
    QVERIFY(addTransaction(QDate(2026, 3, 31), -2000, restaurantes, QStringLiteral("Hasta")) > 0);
    QVERIFY(addTransaction(QDate(2026, 4, 1), -4000, restaurantes, QStringLiteral("Fuera")) > 0);

    PeriodTotals totals;
    QString error;
    QVERIFY2(m_reports->totals({QDate(2026, 3, 1), QDate(2026, 3, 31)}, &totals, &error),
             qPrintable(error));
    QCOMPARE(totals.expenseCents, qint64(-3000));
    QCOMPARE(totals.count, 2);
}

void TestReportRepository::categoryBreakdownDirectAndSubtree()
{
    const int gasto = categoryIdByName(QStringLiteral("Gasto"));
    const int comida = categoryIdByName(QStringLiteral("Comida"));
    const int restaurantes = categoryIdByName(QStringLiteral("Restaurantes"));
    QVERIFY(gasto > 0);
    QVERIFY(comida > 0);
    QVERIFY(restaurantes > 0);

    QVERIFY(addTransaction(QDate(2026, 3, 5), -2000, comida, QStringLiteral("Mercado")) > 0);
    QVERIFY(addTransaction(QDate(2026, 3, 6), -1000, restaurantes, QStringLiteral("Cafe")) > 0);

    QList<CategoryTotal> totals;
    QString error;
    QVERIFY2(m_reports->categoryBreakdown({QDate(2026, 3, 1), QDate(2026, 3, 31)}, gasto,
                                          &totals, &error),
             qPrintable(error));

    const CategoryTotal *comidaTotal = totalById(totals, comida);
    QVERIFY(comidaTotal != nullptr);
    QCOMPARE(comidaTotal->directCents, qint64(-2000));
    QCOMPARE(comidaTotal->subtreeCents, qint64(-3000));
    QCOMPARE(comidaTotal->directCount, 1);
    QCOMPARE(comidaTotal->subtreeCount, 2);

    const CategoryTotal *restaurantesTotal = totalById(totals, restaurantes);
    QVERIFY(restaurantesTotal != nullptr);
    QCOMPARE(restaurantesTotal->directCents, qint64(-1000));
    QCOMPARE(restaurantesTotal->subtreeCents, qint64(-1000));
    QCOMPARE(restaurantesTotal->path, QStringLiteral("Gasto > Comida > Restaurantes"));

    const CategoryTotal *gastoTotal = totalById(totals, gasto);
    QVERIFY(gastoTotal != nullptr);
    QCOMPARE(gastoTotal->directCents, qint64(0));
    QCOMPARE(gastoTotal->subtreeCents, qint64(-3000));
    QCOMPARE(gastoTotal->subtreeCount, 2);
}

void TestReportRepository::categoryBreakdownRestrictedToRoot()
{
    const int comida = categoryIdByName(QStringLiteral("Comida"));
    const int restaurantes = categoryIdByName(QStringLiteral("Restaurantes"));
    const int supermercado = categoryIdByName(QStringLiteral("Supermercado"));
    const int ocio = categoryIdByName(QStringLiteral("Ocio"));
    QVERIFY(comida > 0);
    QVERIFY(restaurantes > 0);
    QVERIFY(supermercado > 0);
    QVERIFY(ocio > 0);

    QVERIFY(addTransaction(QDate(2026, 3, 5), -2000, comida, QStringLiteral("Mercado")) > 0);
    QVERIFY(addTransaction(QDate(2026, 3, 6), -1000, restaurantes, QStringLiteral("Cafe")) > 0);
    QVERIFY(addTransaction(QDate(2026, 3, 7), -5000, ocio, QStringLiteral("Cine")) > 0);

    QList<CategoryTotal> totals;
    QString error;
    QVERIFY2(m_reports->categoryBreakdown({QDate(2026, 3, 1), QDate(2026, 3, 31)}, comida,
                                          &totals, &error),
             qPrintable(error));

    QCOMPARE(totals.size(), 3);
    QVERIFY(totalById(totals, comida) != nullptr);
    QVERIFY(totalById(totals, restaurantes) != nullptr);
    QVERIFY(totalById(totals, supermercado) != nullptr);
    QVERIFY(totalById(totals, ocio) == nullptr);
}

void TestReportRepository::timeSeriesMonthlyBucketsAndCumulative()
{
    const int salario = categoryIdByName(QStringLiteral("Salario"));
    const int restaurantes = categoryIdByName(QStringLiteral("Restaurantes"));
    QVERIFY(salario > 0);
    QVERIFY(restaurantes > 0);

    QVERIFY(addTransaction(QDate(2026, 1, 10), 100000, salario, QStringLiteral("Nomina")) > 0);
    QVERIFY(addTransaction(QDate(2026, 1, 20), -30000, restaurantes, QStringLiteral("Cena")) > 0);
    QVERIFY(addTransaction(QDate(2026, 2, 10), 50000, salario, QStringLiteral("Nomina")) > 0);

    QList<TimeBucket> buckets;
    QString error;
    QVERIFY2(m_reports->timeSeries({QDate(2026, 1, 1), QDate(2026, 2, 28)}, Granularity::Month,
                                   &buckets, &error),
             qPrintable(error));

    QCOMPARE(buckets.size(), 2);
    QCOMPARE(buckets.at(0).start, QDate(2026, 1, 1));
    QCOMPARE(buckets.at(0).incomeCents, qint64(100000));
    QCOMPARE(buckets.at(0).expenseCents, qint64(-30000));
    QCOMPARE(buckets.at(0).netCents, qint64(70000));
    QCOMPARE(buckets.at(0).cumulativeNetCents, qint64(70000));

    QCOMPARE(buckets.at(1).start, QDate(2026, 2, 1));
    QCOMPARE(buckets.at(1).incomeCents, qint64(50000));
    QCOMPARE(buckets.at(1).expenseCents, qint64(0));
    QCOMPARE(buckets.at(1).netCents, qint64(50000));
    QCOMPARE(buckets.at(1).cumulativeNetCents, qint64(120000));
}

void TestReportRepository::timeSeriesFillsEmptyBuckets()
{
    const int salario = categoryIdByName(QStringLiteral("Salario"));
    QVERIFY(salario > 0);
    QVERIFY(addTransaction(QDate(2026, 1, 10), 100000, salario, QStringLiteral("Nomina")) > 0);

    QList<TimeBucket> buckets;
    QString error;
    QVERIFY2(m_reports->timeSeries({QDate(2026, 1, 1), QDate(2026, 3, 31)}, Granularity::Month,
                                   &buckets, &error),
             qPrintable(error));

    QCOMPARE(buckets.size(), 3);
    QCOMPARE(buckets.at(2).start, QDate(2026, 3, 1));
    QCOMPARE(buckets.at(2).netCents, qint64(0));
    QCOMPARE(buckets.at(2).cumulativeNetCents, qint64(100000));
}

void TestReportRepository::timeSeriesWeeksStartOnMonday()
{
    const int restaurantes = categoryIdByName(QStringLiteral("Restaurantes"));
    QVERIFY(restaurantes > 0);

    const QDate date(2026, 3, 4);
    QVERIFY(addTransaction(date, -1500, restaurantes, QStringLiteral("Snack")) > 0);

    QList<TimeBucket> buckets;
    QString error;
    QVERIFY2(m_reports->timeSeries({date, date}, Granularity::Week, &buckets, &error),
             qPrintable(error));

    QCOMPARE(buckets.size(), 1);
    QCOMPARE(buckets.first().start.dayOfWeek(), 1);
    QVERIFY(buckets.first().start <= date);
    QVERIFY(date <= buckets.first().start.addDays(6));
    QCOMPARE(buckets.first().expenseCents, qint64(-1500));
}

QTEST_GUILESS_MAIN(TestReportRepository)
#include "test_reportrepository.moc"
