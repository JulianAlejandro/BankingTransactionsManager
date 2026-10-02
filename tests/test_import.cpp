#include <QtTest>

#include <memory>

#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>
#include <QXmlStreamWriter>

#include "data/categoryrepository.h"
#include "data/database.h"
#include "data/import/importutils.h"
#include "data/importservice.h"
#include "data/transactionservice.h"
#include "model/transaction.h"
#include "model/derivedtransaction.h"

#include <private/qzipwriter_p.h>

namespace {

QString writeCsvFixture(const QString &dir)
{
    const QString path = dir + QStringLiteral("/extracto.csv");
    QFile file(path);
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << "F.Valor;Fecha;Concepto;Movimiento;Importe;Divisa\n";
    out << "02/10/2026;02/10/2026;Bizum;Recibido: comida;8.50;EUR\n";
    out << "29/09/2026;01/10/2026;Abono de nomina;Weidmuller, s.a.;1803.89;EUR\n";
    out << "29/09/2026;01/10/2026;Revolut**7187*;Pago con tarjeta;-600;EUR\n";
    return path;
}

// Crea un XLSX minimo (ZIP con las partes necesarias) usando QZipWriter.
QString writeXlsxFixture(const QString &dir)
{
    const QString path = dir + QStringLiteral("/extracto.xlsx");

    const QByteArray contentTypes =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
        "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
        "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
        "<Override PartName=\"/xl/workbook.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml\"/>"
        "<Override PartName=\"/xl/worksheets/sheet1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/>"
        "<Override PartName=\"/xl/sharedStrings.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.sharedStrings+xml\"/>"
        "</Types>";

    const QByteArray rels =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"xl/workbook.xml\"/>"
        "</Relationships>";

    const QByteArray workbook =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" "
        "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">"
        "<sheets><sheet name=\"Informe\" sheetId=\"1\" r:id=\"rId1\"/></sheets></workbook>";

    QStringList shared;
    shared << QStringLiteral("F.Valor") << QStringLiteral("Fecha") << QStringLiteral("Concepto")
           << QStringLiteral("Movimiento") << QStringLiteral("Importe")
           << QStringLiteral("02/10/2026") << QStringLiteral("Bizum")
           << QStringLiteral("Recibido: comida") << QStringLiteral("29/09/2026")
           << QStringLiteral("Abono de nomina");

    QString sharedXml;
    {
        QXmlStreamWriter xml(&sharedXml);
        xml.writeStartDocument();
        xml.writeStartElement(QStringLiteral("sst"));
        xml.writeAttribute(QStringLiteral("xmlns"),
                           QStringLiteral("http://schemas.openxmlformats.org/spreadsheetml/2006/main"));
        for (const QString &value : shared) {
            xml.writeStartElement(QStringLiteral("si"));
            xml.writeTextElement(QStringLiteral("t"), value);
            xml.writeEndElement();
        }
        xml.writeEndElement();
        xml.writeEndDocument();
    }

    const QByteArray sheet =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">"
        "<sheetData>"
        "<row r=\"1\">"
        "<c r=\"B1\" t=\"s\"><v>0</v></c>"
        "<c r=\"C1\" t=\"s\"><v>1</v></c>"
        "<c r=\"D1\" t=\"s\"><v>2</v></c>"
        "<c r=\"E1\" t=\"s\"><v>3</v></c>"
        "<c r=\"F1\" t=\"s\"><v>4</v></c>"
        "</row>"
        "<row r=\"2\">"
        "<c r=\"C2\" t=\"s\"><v>5</v></c>"
        "<c r=\"D2\" t=\"s\"><v>6</v></c>"
        "<c r=\"E2\" t=\"s\"><v>7</v></c>"
        "<c r=\"F2\"><v>8.5</v></c>"
        "</row>"
        "<row r=\"3\">"
        "<c r=\"C3\" t=\"s\"><v>8</v></c>"
        "<c r=\"D3\" t=\"s\"><v>9</v></c>"
        "<c r=\"F3\"><v>1803.89</v></c>"
        "</row>"
        "</sheetData></worksheet>";

    QZipWriter zip(path);
    zip.addFile(QStringLiteral("[Content_Types].xml"), contentTypes);
    zip.addFile(QStringLiteral("_rels/.rels"), rels);
    zip.addFile(QStringLiteral("xl/workbook.xml"), workbook);
    zip.addFile(QStringLiteral("xl/sharedStrings.xml"), sharedXml.toUtf8());
    zip.addFile(QStringLiteral("xl/worksheets/sheet1.xml"), sheet);
    zip.close();
    return path;
}

} // namespace

class TestImport : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void utilsParseDateAndAmount();
    void utilsCombineConcept();
    void csvReaderReadsMovements();
    void xlsxReaderReadsMovements();
    void serviceMarksDuplicates();
    void serviceImportsSelected();

private:
    Database m_db;
    std::unique_ptr<ImportService> m_import;
    std::unique_ptr<TransactionService> m_transactions;

    QTemporaryDir m_dir;
};

void TestImport::init()
{
    QString error;
    QVERIFY2(m_db.open(QStringLiteral(":memory:"), &error), qPrintable(error));
    m_import = std::make_unique<ImportService>(m_db.connection());
    m_transactions = std::make_unique<TransactionService>(m_db.connection());
    QVERIFY(m_dir.isValid());
}

void TestImport::cleanup()
{
    m_import.reset();
    m_transactions.reset();
    m_db.close();
}

void TestImport::utilsParseDateAndAmount()
{
    QCOMPARE(ImportUtils::parseDate(QStringLiteral("02/10/2026")), QDate(2026, 10, 2));
    QCOMPARE(ImportUtils::parseDate(QStringLiteral("2026-10-02")), QDate(2026, 10, 2));
    QVERIFY(!ImportUtils::parseDate(QStringLiteral("")).isValid());

    bool ok = false;
    QCOMPARE(ImportUtils::parseAmountCents(QStringLiteral("8.50"), &ok), qint64(850));
    QVERIFY(ok);
    QCOMPARE(ImportUtils::parseAmountCents(QStringLiteral("-600"), &ok), qint64(-60000));
    QVERIFY(ok);
    QCOMPARE(ImportUtils::parseAmountCents(QStringLiteral("1803.89"), &ok), qint64(180389));
    QVERIFY(ok);
}

void TestImport::utilsCombineConcept()
{
    QCOMPARE(ImportUtils::combineConcept(QStringLiteral("Bizum"), QStringLiteral("Recibido: comida")),
             QStringLiteral("Bizum - Recibido: comida"));
    QCOMPARE(ImportUtils::combineConcept(QStringLiteral("Netflix.com"), QStringLiteral("Netflix.com")),
             QStringLiteral("Netflix.com"));
    QCOMPARE(ImportUtils::combineConcept(QStringLiteral("Pizza i diversio"),
                                         QStringLiteral("pizza i diversio")),
             QStringLiteral("Pizza i diversio"));
}

void TestImport::csvReaderReadsMovements()
{
    const QString path = writeCsvFixture(m_dir.path());
    QList<ImportedMovement> movements;
    QString error;
    QVERIFY2(m_import->load(path, &movements, &error), qPrintable(error));
    QCOMPARE(movements.size(), 3);
    QCOMPARE(movements.first().date, QDate(2026, 10, 2));
    QCOMPARE(movements.first().concept, QStringLiteral("Bizum - Recibido: comida"));
    QCOMPARE(movements.first().amountCents, qint64(850));
    QCOMPARE(movements.at(2).amountCents, qint64(-60000));
}

void TestImport::xlsxReaderReadsMovements()
{
    const QString path = writeXlsxFixture(m_dir.path());
    QList<ImportedMovement> movements;
    QString error;
    QVERIFY2(m_import->load(path, &movements, &error), qPrintable(error));
    QCOMPARE(movements.size(), 2);
    QCOMPARE(movements.first().date, QDate(2026, 10, 2));
    QCOMPARE(movements.first().concept, QStringLiteral("Bizum - Recibido: comida"));
    QCOMPARE(movements.first().amountCents, qint64(850));
    QCOMPARE(movements.at(1).amountCents, qint64(180389));
    QVERIFY(movements.at(1).error.isEmpty());
}

void TestImport::serviceMarksDuplicates()
{
    const QString path = writeCsvFixture(m_dir.path());

    // "Revolut**7187* - Pago con tarjeta" es un gasto (-600) -> categoria de gasto.
    CategoryRepository categories(m_db.connection());
    int expenseId = categories.all().first().id;
    for (const Category &category : categories.all()) {
        if (category.name == QStringLiteral("Supermercado")) {
            expenseId = category.id;
        }
    }

    Transaction transaction;
    transaction.date = QDate(2026, 10, 1);
    transaction.concept = QStringLiteral("Revolut**7187* - Pago con tarjeta");
    transaction.amountCents = -60000;
    DerivedTransaction derived;
    derived.categoryId = expenseId;
    derived.amountCents = -60000;
    QList<DerivedTransaction> derivedList;
    derivedList << derived;
    QString error;
    QVERIFY2(m_transactions->create(transaction, derivedList, nullptr, &error), qPrintable(error));

    QList<ImportedMovement> movements;
    QVERIFY2(m_import->load(path, &movements, &error), qPrintable(error));
    QCOMPARE(movements.size(), 3);
    QVERIFY(!movements.at(0).duplicate);
    QVERIFY(!movements.at(1).duplicate);
    QVERIFY(movements.at(2).duplicate);
}

void TestImport::serviceImportsSelected()
{
    const QString path = writeCsvFixture(m_dir.path());
    QList<ImportedMovement> movements;
    QString error;
    QVERIFY2(m_import->load(path, &movements, &error), qPrintable(error));

    CategoryRepository categories(m_db.connection());
    const QList<Category> all = categories.all();
    QVERIFY(!all.isEmpty());
    int expenseId = -1;
    for (const Category &category : all) {
        if (category.name == QStringLiteral("Supermercado")) {
            expenseId = category.id;
        }
    }
    QVERIFY(expenseId > 0);

    ImportResult result;
    QVERIFY2(m_import->importMovements(movements, {2}, expenseId, &result, &error),
             qPrintable(error));
    QCOMPARE(result.imported, 1);
    QCOMPARE(result.failed, 0);
    QCOMPARE(m_transactions->all().size(), 1);
    QCOMPARE(m_transactions->all().first().amountCents, qint64(-60000));
}

QTEST_GUILESS_MAIN(TestImport)
#include "test_import.moc"
