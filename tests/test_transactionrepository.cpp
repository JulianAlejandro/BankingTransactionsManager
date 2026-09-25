#include <QtTest>

#include <memory>

#include "data/categoryrepository.h"
#include "data/database.h"
#include "data/transactionrepository.h"
#include "model/transaction.h"

class TestTransactionRepository : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void addAssignsIdAndPersists();
    void updateChangesAmount();
    void removeDeletesRow();
    void addRejectsInvalidTransaction();
    void categoryInUseCannotBeRemoved();

private:
    int idByName(const QString &name) const;

    Database m_db;
    std::unique_ptr<CategoryRepository> m_categories;
    std::unique_ptr<TransactionRepository> m_transactions;
};

void TestTransactionRepository::init()
{
    QString error;
    QVERIFY2(m_db.open(QStringLiteral(":memory:"), &error), qPrintable(error));
    m_categories = std::make_unique<CategoryRepository>(m_db.connection());
    m_transactions = std::make_unique<TransactionRepository>(m_db.connection());
}

void TestTransactionRepository::cleanup()
{
    m_transactions.reset();
    m_categories.reset();
    m_db.close();
}

int TestTransactionRepository::idByName(const QString &name) const
{
    const QList<Category> categories = m_categories->all();
    for (const Category &category : categories) {
        if (category.name == name) {
            return category.id;
        }
    }
    return -1;
}

void TestTransactionRepository::addAssignsIdAndPersists()
{
    Transaction transaction;
    transaction.date = QDate(2026, 9, 25);
    transaction.concept = QStringLiteral("Compra semanal");
    transaction.categoryId = idByName(QStringLiteral("Supermercado"));
    transaction.amountCents = 4599;
    transaction.account = QStringLiteral("Cuenta principal");

    QString error;
    QVERIFY2(m_transactions->add(transaction, &error), qPrintable(error));
    QVERIFY(transaction.id > 0);

    const QList<Transaction> stored = m_transactions->all();
    QCOMPARE(stored.size(), 1);
    QCOMPARE(stored.first().concept, QStringLiteral("Compra semanal"));
    QCOMPARE(stored.first().amountCents, qint64(4599));
    QCOMPARE(stored.first().categoryId, transaction.categoryId);
}

void TestTransactionRepository::updateChangesAmount()
{
    Transaction transaction;
    transaction.date = QDate(2026, 1, 10);
    transaction.concept = QStringLiteral("Cafe");
    transaction.categoryId = idByName(QStringLiteral("Restaurantes"));
    transaction.amountCents = 250;

    QString error;
    QVERIFY2(m_transactions->add(transaction, &error), qPrintable(error));

    transaction.amountCents = 300;
    transaction.concept = QStringLiteral("Cafe con leche");
    QVERIFY2(m_transactions->update(transaction, &error), qPrintable(error));

    const QList<Transaction> stored = m_transactions->all();
    QCOMPARE(stored.size(), 1);
    QCOMPARE(stored.first().amountCents, qint64(300));
    QCOMPARE(stored.first().concept, QStringLiteral("Cafe con leche"));
}

void TestTransactionRepository::removeDeletesRow()
{
    Transaction transaction;
    transaction.date = QDate(2026, 2, 1);
    transaction.concept = QStringLiteral("Bus");
    transaction.categoryId = idByName(QStringLiteral("Transporte"));
    transaction.amountCents = 150;

    QString error;
    QVERIFY2(m_transactions->add(transaction, &error), qPrintable(error));
    QVERIFY2(m_transactions->remove(transaction.id, &error), qPrintable(error));
    QCOMPARE(m_transactions->all().size(), 0);
}

void TestTransactionRepository::addRejectsInvalidTransaction()
{
    Transaction transaction;
    transaction.date = QDate(2026, 2, 1);
    transaction.concept = QStringLiteral("Sin importe");
    transaction.categoryId = idByName(QStringLiteral("Transporte"));
    transaction.amountCents = 0;

    QString error;
    QVERIFY(!m_transactions->add(transaction, &error));
    QVERIFY(!error.isEmpty());
    QCOMPARE(m_transactions->all().size(), 0);
}

void TestTransactionRepository::categoryInUseCannotBeRemoved()
{
    const int transporte = idByName(QStringLiteral("Transporte"));

    Transaction transaction;
    transaction.date = QDate(2026, 3, 3);
    transaction.concept = QStringLiteral("Taxi");
    transaction.categoryId = transporte;
    transaction.amountCents = 1200;

    QString error;
    QVERIFY2(m_transactions->add(transaction, &error), qPrintable(error));
    QVERIFY(!m_categories->remove(transporte, &error));
    QVERIFY(!error.isEmpty());
}

QTEST_GUILESS_MAIN(TestTransactionRepository)
#include "test_transactionrepository.moc"
