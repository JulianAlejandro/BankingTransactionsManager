#include <QtTest>

#include <memory>

#include "data/categoryrepository.h"
#include "data/database.h"

class TestCategoryRepository : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void seedHasTwoRoots();
    void pathIsBuiltFromAncestors();
    void addChildInheritsParentType();
    void moveIntoOwnDescendantFails();
    void moveToOtherBranchFails();
    void removeCategoryWithChildrenFails();

private:
    int idByName(const QString &name) const;

    Database m_db;
    std::unique_ptr<CategoryRepository> m_repo;
};

void TestCategoryRepository::init()
{
    QString error;
    QVERIFY2(m_db.open(QStringLiteral(":memory:"), &error), qPrintable(error));
    m_repo = std::make_unique<CategoryRepository>(m_db.connection());
}

void TestCategoryRepository::cleanup()
{
    m_repo.reset();
    m_db.close();
}

int TestCategoryRepository::idByName(const QString &name) const
{
    const QList<Category> categories = m_repo->all();
    for (const Category &category : categories) {
        if (category.name == name) {
            return category.id;
        }
    }
    return -1;
}

void TestCategoryRepository::seedHasTwoRoots()
{
    const QList<Category> categories = m_repo->all();
    QVERIFY(!categories.isEmpty());

    int roots = 0;
    for (const Category &category : categories) {
        if (category.parentId < 0) {
            ++roots;
        }
    }
    QCOMPARE(roots, 2);
}

void TestCategoryRepository::pathIsBuiltFromAncestors()
{
    const int restaurantes = idByName(QStringLiteral("Restaurantes"));
    QVERIFY(restaurantes > 0);

    bool ok = false;
    const QString path = m_repo->pathOf(restaurantes, &ok);
    QVERIFY(ok);
    QCOMPARE(path, QStringLiteral("Gasto > Comida > Restaurantes"));

    const QList<Category> categories = m_repo->withPaths();
    for (const Category &category : categories) {
        if (category.id == restaurantes) {
            QCOMPARE(category.depth, 2);
        }
    }
}

void TestCategoryRepository::addChildInheritsParentType()
{
    const int comida = idByName(QStringLiteral("Comida"));
    QVERIFY(comida > 0);

    Category created;
    QString error;
    // Se pasa Expense a proposito; debe heredarse el tipo del padre.
    QVERIFY2(m_repo->add(comida, QStringLiteral("Panaderia"), CategoryType::Income,
                         &created, &error),
             qPrintable(error));
    QCOMPARE(created.type, CategoryType::Expense);
    QCOMPARE(created.parentId, comida);

    bool ok = false;
    const QString path = m_repo->pathOf(created.id, &ok);
    QVERIFY(ok);
    QCOMPARE(path, QStringLiteral("Gasto > Comida > Panaderia"));
}

void TestCategoryRepository::moveIntoOwnDescendantFails()
{
    const int gasto = idByName(QStringLiteral("Gasto"));
    const int comida = idByName(QStringLiteral("Comida"));
    QVERIFY(gasto > 0);
    QVERIFY(comida > 0);

    QString error;
    QVERIFY(!m_repo->move(gasto, comida, &error));
    QVERIFY(!error.isEmpty());
}

void TestCategoryRepository::moveToOtherBranchFails()
{
    const int comida = idByName(QStringLiteral("Comida"));
    const int salario = idByName(QStringLiteral("Salario"));
    QVERIFY(comida > 0);
    QVERIFY(salario > 0);

    QString error;
    QVERIFY(!m_repo->move(comida, salario, &error));
    QVERIFY(!error.isEmpty());
}

void TestCategoryRepository::removeCategoryWithChildrenFails()
{
    const int comida = idByName(QStringLiteral("Comida"));
    QVERIFY(comida > 0);

    QString error;
    QVERIFY(!m_repo->remove(comida, &error));
    QVERIFY(!error.isEmpty());
}

QTEST_GUILESS_MAIN(TestCategoryRepository)
#include "test_categoryrepository.moc"
