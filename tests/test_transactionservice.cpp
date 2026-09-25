#include <QtTest>

#include <memory>

#include "data/categoryrepository.h"
#include "data/database.h"
#include "data/transactionservice.h"
#include "model/derivedtransaction.h"
#include "model/transaction.h"

class TestTransactionService : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void createWithDefaultDerivedPersists();
    void createRejectsUnbalanced();
    void createRejectsWithoutDerived();
    void createRejectsWrongSign();
    void saveReplacesBreakdown();
    void removeCascadesDerived();
    void categoryInUseCannotBeRemoved();

private:
    int idByName(const QString &name) const;
    DerivedTransaction expense(int categoryId, qint64 magnitude, const QString &concept) const;
    DerivedTransaction income(int categoryId, qint64 magnitude, const QString &concept) const;

    Database m_db;
    std::unique_ptr<CategoryRepository> m_categories;
    std::unique_ptr<TransactionService> m_service;
};

void TestTransactionService::init()
{
    QString error;
    QVERIFY2(m_db.open(QStringLiteral(":memory:"), &error), qPrintable(error));
    m_categories = std::make_unique<CategoryRepository>(m_db.connection());
    m_service = std::make_unique<TransactionService>(m_db.connection());
}

void TestTransactionService::cleanup()
{
    m_service.reset();
    m_categories.reset();
    m_db.close();
}

int TestTransactionService::idByName(const QString &name) const
{
    const QList<Category> categories = m_categories->all();
    for (const Category &category : categories) {
        if (category.name == name) {
            return category.id;
        }
    }
    return -1;
}

DerivedTransaction TestTransactionService::expense(int categoryId, qint64 magnitude,
                                                   const QString &concept) const
{
    DerivedTransaction derived;
    derived.categoryId = categoryId;
    derived.amountCents = signedAmountFor(magnitude, CategoryType::Expense);
    derived.concept = concept;
    return derived;
}

DerivedTransaction TestTransactionService::income(int categoryId, qint64 magnitude,
                                                  const QString &concept) const
{
    DerivedTransaction derived;
    derived.categoryId = categoryId;
    derived.amountCents = signedAmountFor(magnitude, CategoryType::Income);
    derived.concept = concept;
    return derived;
}

void TestTransactionService::createWithDefaultDerivedPersists()
{
    Transaction transaction;
    transaction.date = QDate(2026, 9, 25);
    transaction.concept = QStringLiteral("Bizum a Luis");
    transaction.amountCents = -10000;
    transaction.account = QStringLiteral("Cuenta principal");

    QList<DerivedTransaction> derived;
    derived << expense(idByName(QStringLiteral("Restaurantes")), 10000, transaction.concept);

    QString error;
    QVERIFY2(m_service->create(transaction, derived, nullptr, &error), qPrintable(error));
    QVERIFY(transaction.id > 0);

    Transaction loaded;
    QList<DerivedTransaction> loadedDerived;
    QVERIFY2(m_service->load(transaction.id, &loaded, &loadedDerived, &error), qPrintable(error));
    QCOMPARE(loaded.amountCents, qint64(-10000));
    QCOMPARE(loadedDerived.size(), 1);
    QCOMPARE(loadedDerived.first().amountCents, qint64(-10000));
    QCOMPARE(loadedDerived.first().transactionId, transaction.id);
}

void TestTransactionService::createRejectsUnbalanced()
{
    Transaction transaction;
    transaction.date = QDate(2026, 9, 25);
    transaction.concept = QStringLiteral("Bizum a Luis");
    transaction.amountCents = -10000;

    QList<DerivedTransaction> derived;
    derived << expense(idByName(QStringLiteral("Restaurantes")), 5000, QStringLiteral("Cena"));

    QString error;
    QVERIFY(!m_service->create(transaction, derived, nullptr, &error));
    QVERIFY(!error.isEmpty());
    QCOMPARE(m_service->all().size(), 0);
}

void TestTransactionService::createRejectsWithoutDerived()
{
    Transaction transaction;
    transaction.date = QDate(2026, 9, 25);
    transaction.concept = QStringLiteral("Bizum");
    transaction.amountCents = -10000;

    QList<DerivedTransaction> derived;

    QString error;
    QVERIFY(!m_service->create(transaction, derived, nullptr, &error));
    QVERIFY(!error.isEmpty());
}

void TestTransactionService::createRejectsWrongSign()
{
    Transaction transaction;
    transaction.date = QDate(2026, 9, 25);
    transaction.concept = QStringLiteral("Bizum");
    transaction.amountCents = 10000;

    QList<DerivedTransaction> derived;
    // Categoria de gasto con importe positivo -> invalido.
    DerivedTransaction wrong;
    wrong.categoryId = idByName(QStringLiteral("Restaurantes"));
    wrong.amountCents = 10000;
    derived << wrong;

    QString error;
    QVERIFY(!m_service->create(transaction, derived, nullptr, &error));
    QVERIFY(!error.isEmpty());
}

void TestTransactionService::saveReplacesBreakdown()
{
    Transaction transaction;
    transaction.date = QDate(2026, 9, 25);
    transaction.concept = QStringLiteral("Bizum a Luis");
    transaction.amountCents = -10000;

    QList<DerivedTransaction> derived;
    derived << expense(idByName(QStringLiteral("Restaurantes")), 10000, QStringLiteral("Cena"));

    QString error;
    QVERIFY2(m_service->create(transaction, derived, nullptr, &error), qPrintable(error));

    Transaction loaded;
    QList<DerivedTransaction> loadedDerived;
    QVERIFY2(m_service->load(transaction.id, &loaded, &loadedDerived, &error), qPrintable(error));

    QList<DerivedTransaction> split;
    split << expense(idByName(QStringLiteral("Restaurantes")), 5000, QStringLiteral("Cena"));
    split << income(idByName(QStringLiteral("Otros")), 2000, QStringLiteral("Le devuelvo"));
    split << expense(idByName(QStringLiteral("Ocio")), 7000, QStringLiteral("Regalo"));

    QVERIFY2(m_service->save(loaded, split, &error), qPrintable(error));

    QList<DerivedTransaction> reloaded;
    QVERIFY2(m_service->load(transaction.id, &loaded, &reloaded, &error), qPrintable(error));
    QCOMPARE(reloaded.size(), 3);
    qint64 sum = 0;
    for (const DerivedTransaction &item : reloaded) {
        sum += item.amountCents;
    }
    QCOMPARE(sum, qint64(-10000));
}

void TestTransactionService::removeCascadesDerived()
{
    Transaction transaction;
    transaction.date = QDate(2026, 9, 25);
    transaction.concept = QStringLiteral("Bizum");
    transaction.amountCents = -10000;

    QList<DerivedTransaction> derived;
    derived << expense(idByName(QStringLiteral("Restaurantes")), 10000, QStringLiteral("Cena"));

    QString error;
    QVERIFY2(m_service->create(transaction, derived, nullptr, &error), qPrintable(error));
    const int id = transaction.id;

    QVERIFY2(m_service->remove(id, &error), qPrintable(error));
    QCOMPARE(m_service->all().size(), 0);

    QList<DerivedTransaction> remaining;
    QVERIFY(!m_service->load(id, nullptr, &remaining, &error));
}

void TestTransactionService::categoryInUseCannotBeRemoved()
{
    const int restaurantes = idByName(QStringLiteral("Restaurantes"));

    Transaction transaction;
    transaction.date = QDate(2026, 9, 25);
    transaction.concept = QStringLiteral("Cena");
    transaction.amountCents = -10000;

    QList<DerivedTransaction> derived;
    derived << expense(restaurantes, 10000, QStringLiteral("Cena"));

    QString error;
    QVERIFY2(m_service->create(transaction, derived, nullptr, &error), qPrintable(error));

    QVERIFY(!m_categories->remove(restaurantes, &error));
    QVERIFY(!error.isEmpty());
}

QTEST_GUILESS_MAIN(TestTransactionService)
#include "test_transactionservice.moc"
