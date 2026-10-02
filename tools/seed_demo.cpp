#include <QCoreApplication>
#include <QDate>
#include <QHash>
#include <QList>
#include <QString>
#include <QTextStream>

#include "appinfo.h"
#include "data/categoryrepository.h"
#include "data/database.h"
#include "data/transactionservice.h"
#include "model/category.h"
#include "model/derivedtransaction.h"
#include "model/transaction.h"

namespace {

struct Part
{
    QString category;
    qint64 magnitudeCents;
};

QTextStream &out()
{
    static QTextStream stream(stdout);
    return stream;
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName(AppInfo::Organization);
    QCoreApplication::setApplicationName(AppInfo::Name);

    Database database;
    QString error;
    if (!database.openDefault(&error)) {
        out() << "No se pudo abrir la base de datos: " << error << Qt::endl;
        return 1;
    }

    CategoryRepository categories(database.connection());
    TransactionService service(database.connection());

    const QList<Transaction> existing = service.all(&error);
    if (!existing.isEmpty()) {
        out() << "La base de datos ya tiene " << existing.size()
              << " transacciones. Borra transacciones.db si quieres sembrar de nuevo."
              << Qt::endl;
        return 0;
    }

    QHash<QString, Category> byName;
    const QList<Category> all = categories.all(&error);
    if (!error.isEmpty()) {
        out() << error << Qt::endl;
        return 1;
    }
    for (const Category &category : all) {
        byName.insert(category.name, category);
    }

    const auto create = [&](const QString &concept, const QDate &date,
                            const QList<Part> &parts) -> bool {
        QList<DerivedTransaction> derived;
        qint64 total = 0;
        for (const Part &part : parts) {
            const auto it = byName.constFind(part.category);
            if (it == byName.constEnd()) {
                out() << "Categoria desconocida: " << part.category << Qt::endl;
                return false;
            }
            DerivedTransaction item;
            item.categoryId = it->id;
            item.concept = concept;
            item.amountCents = signedAmountFor(part.magnitudeCents, it->type);
            derived.append(item);
            total += item.amountCents;
        }

        Transaction transaction;
        transaction.date = date;
        transaction.concept = concept;
        transaction.amountCents = total;

        QString createError;
        if (!service.create(transaction, derived, nullptr, &createError)) {
            out() << "Fallo al crear '" << concept << "': " << createError << Qt::endl;
            return false;
        }
        return true;
    };

    bool ok = true;
    ok = ok && create(QStringLiteral("Nomina enero"), QDate(2026, 1, 3),
                      {{QStringLiteral("Fijo"), 200000}});
    ok = ok && create(QStringLiteral("Compra semanal"), QDate(2026, 1, 5),
                      {{QStringLiteral("Supermercado"), 6000}});
    ok = ok && create(QStringLiteral("Cena"), QDate(2026, 1, 12),
                      {{QStringLiteral("Restaurantes"), 4500}});
    ok = ok && create(QStringLiteral("Alquiler enero"), QDate(2026, 1, 20),
                      {{QStringLiteral("Alquiler"), 80000}});
    ok = ok && create(QStringLiteral("Cine"), QDate(2026, 1, 25),
                      {{QStringLiteral("Ocio"), 1500}});
    ok = ok && create(QStringLiteral("Nomina febrero"), QDate(2026, 2, 3),
                      {{QStringLiteral("Fijo"), 200000}});
    ok = ok && create(QStringLiteral("Supermercado"), QDate(2026, 2, 8),
                      {{QStringLiteral("Supermercado"), 7200}});
    ok = ok && create(QStringLiteral("Viaje fin de semana"), QDate(2026, 2, 14),
                      {{QStringLiteral("Restaurantes"), 5000},
                       {QStringLiteral("Transporte"), 4000},
                       {QStringLiteral("Ocio"), 6000}});
    ok = ok && create(QStringLiteral("Pan y fruta"), QDate(2026, 2, 27),
                      {{QStringLiteral("Comida"), 3000}});
    ok = ok && create(QStringLiteral("Nomina marzo"), QDate(2026, 3, 3),
                      {{QStringLiteral("Fijo"), 200000}});
    ok = ok && create(QStringLiteral("Venta bici"), QDate(2026, 3, 9),
                      {{QStringLiteral("Otros"), 30000}});
    ok = ok && create(QStringLiteral("Supermercado"), QDate(2026, 3, 15),
                      {{QStringLiteral("Supermercado"), 8000}});
    ok = ok && create(QStringLiteral("Farmacia"), QDate(2026, 3, 22),
                      {{QStringLiteral("Salud"), 2500}});

    if (!ok) {
        return 1;
    }

    out() << "Datos de ejemplo creados (13 transacciones)." << Qt::endl;
    return 0;
}
