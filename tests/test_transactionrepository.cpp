#include <QtTest>

#include <memory>

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

private:
    Database m_db;
    std::unique_ptr<TransactionRepository> m_transactions;
};

void TestTransactionRepository::init()
{
    QString error;
    QVERIFY2(m_db.open(QStringLiteral(":memory:"), &error), qPrintable(error));
    m_transactions = std::make_unique<TransactionRepository>(m_db.connection());
}

void TestTransactionRepository::cleanup()
{
    m_transactions.reset();
    m_db.close();
}

void TestTransactionRepository::addAssignsIdAndPersists()
{
    Transaction transaction;
    transaction.date = QDate(2026, 9, 25);
    transaction.concept = QStringLiteral("Compra semanal");
    transaction.amountCents = -4599;
    transaction.account = QStringLiteral("Cuenta principal");

    QString error;
    QVERIFY2(m_transactions->add(transaction, &error), qPrintable(error));
    QVERIFY(transaction.id > 0);

    const QList<Transaction> stored = m_transactions->all();
    QCOMPARE(stored.size(), 1);
    QCOMPARE(stored.first().concept, QStringLiteral("Compra semanal"));
    QCOMPARE(stored.first().amountCents, qint64(-4599));
}

void TestTransactionRepository::updateChangesAmount()
{
    Transaction transaction;
    transaction.date = QDate(2026, 1, 10);
    transaction.concept = QStringLiteral("Cafe");
    transaction.amountCents = -250;

    QString error;
    QVERIFY2(m_transactions->add(transaction, &error), qPrintable(error));

    transaction.amountCents = -300;
    transaction.concept = QStringLiteral("Cafe con leche");
    QVERIFY2(m_transactions->update(transaction, &error), qPrintable(error));

    const QList<Transaction> stored = m_transactions->all();
    QCOMPARE(stored.size(), 1);
    QCOMPARE(stored.first().amountCents, qint64(-300));
    QCOMPARE(stored.first().concept, QStringLiteral("Cafe con leche"));
}

void TestTransactionRepository::removeDeletesRow()
{
    Transaction transaction;
    transaction.date = QDate(2026, 2, 1);
    transaction.concept = QStringLiteral("Bus");
    transaction.amountCents = -150;

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
    transaction.amountCents = 0;

    QString error;
    QVERIFY(!m_transactions->add(transaction, &error));
    QVERIFY(!error.isEmpty());
    QCOMPARE(m_transactions->all().size(), 0);
}

QTEST_GUILESS_MAIN(TestTransactionRepository)
#include "test_transactionrepository.moc"
