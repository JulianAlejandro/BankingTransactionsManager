#include "data/importservice.h"

#include <memory>
#include <utility>

#include <QFileInfo>

#include "data/import/csvimportreader.h"
#include "data/import/importreader.h"
#include "data/import/xlsximportreader.h"
#include "data/transactionservice.h"
#include "model/derivedtransaction.h"
#include "model/transaction.h"

namespace {

bool contentEqual(const QString &a, const QString &b)
{
    return a.trimmed().compare(b.trimmed(), Qt::CaseInsensitive) == 0;
}

bool exists(const Transaction &transaction, qint64 amountCents, const QString &concept,
            const QList<Transaction> &existing)
{
    for (const Transaction &candidate : existing) {
        if (candidate.date == transaction.date
            && candidate.amountCents == amountCents
            && contentEqual(candidate.concept, concept)) {
            return true;
        }
    }
    return false;
}

} // namespace

ImportService::ImportService(QSqlDatabase db)
    : m_db(std::move(db))
{
}

bool ImportService::load(const QString &path, QList<ImportedMovement> *out,
                         QString *error) const
{
    if (out == nullptr) {
        if (error) *error = QStringLiteral("La salida no puede ser nula.");
        return false;
    }

    const QString suffix = QFileInfo(path).suffix().toLower();
    std::unique_ptr<ImportReader> reader;
    if (suffix == QLatin1String("csv")) {
        reader = std::make_unique<CsvImportReader>();
    } else if (suffix == QLatin1String("xlsx")) {
        reader = std::make_unique<XlsxImportReader>();
    } else {
        if (error) *error = QStringLiteral("Formato no soportado (usa .xlsx o .csv).");
        return false;
    }

    if (!reader->read(path, out, error)) {
        return false;
    }

    // Deduplicacion contra las transacciones existentes.
    TransactionService service(m_db);
    const QList<Transaction> existing = service.all(error);
    if (error && !error->isEmpty()) {
        return false;
    }
    for (ImportedMovement &movement : *out) {
        if (!movement.error.isEmpty()) {
            continue;
        }
        Transaction transaction;
        transaction.date = movement.date;
        transaction.concept = movement.concept;
        transaction.amountCents = movement.amountCents;
        movement.duplicate = exists(transaction, movement.amountCents, movement.concept, existing);
    }
    return true;
}

bool ImportService::importMovements(const QList<ImportedMovement> &movements,
                                    const QList<int> &selectedIndexes, int categoryId,
                                    ImportResult *result, QString *error) const
{
    if (result == nullptr) {
        if (error) *error = QStringLiteral("El resultado no puede ser nulo.");
        return false;
    }
    if (categoryId < 0) {
        if (error) *error = QStringLiteral("Selecciona una categoria.");
        return false;
    }

    TransactionService service(m_db);
    *result = ImportResult();

    for (int index : selectedIndexes) {
        if (index < 0 || index >= movements.size()) {
            continue;
        }
        const ImportedMovement &source = movements.at(index);
        if (!source.error.isEmpty() || source.duplicate) {
            ++result->skipped;
            continue;
        }

        Transaction transaction;
        transaction.date = source.date;
        transaction.concept = source.concept;
        transaction.amountCents = source.amountCents;
        transaction.account = source.account;

        DerivedTransaction derived;
        derived.categoryId = categoryId;
        derived.concept = source.concept;
        derived.amountCents = source.amountCents;

        QList<DerivedTransaction> derivedList;
        derivedList << derived;

        QString createError;
        if (service.create(transaction, derivedList, nullptr, &createError)) {
            ++result->imported;
        } else {
            ++result->failed;
            if (result->failed == 1 && error) {
                *error = createError;
            }
        }
    }

    return true;
}
