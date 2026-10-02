#ifndef IMPORTSERVICE_H
#define IMPORTSERVICE_H

#include <QList>
#include <QSqlDatabase>
#include <QString>

#include "data/importtypes.h"

// Lee extractos bancarios (XLSX/CSV), marca duplicados frente a la base de datos
// y crea las transacciones (con una derivada por defecto) de forma atomica.
class ImportService
{
public:
    explicit ImportService(QSqlDatabase db = QSqlDatabase());

    // Lee el fichero segun su extension y marca los movimientos que ya existen.
    bool load(const QString &path, QList<ImportedMovement> *out, QString *error = nullptr) const;

    // Crea las transacciones a partir de los movimientos indicados (indices de
    // 'movements'), todos con una derivada por defecto en 'categoryId'.
    bool importMovements(const QList<ImportedMovement> &movements,
                         const QList<int> &selectedIndexes, int categoryId,
                         ImportResult *result, QString *error = nullptr) const;

private:
    QSqlDatabase m_db;
};

#endif // IMPORTSERVICE_H
