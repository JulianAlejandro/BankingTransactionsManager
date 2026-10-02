#ifndef IMPORTTYPES_H
#define IMPORTTYPES_H

#include <QDate>
#include <QString>

// Un movimiento leido de un fichero externo (extracto bancario) antes de
// convertirse en una transaccion del sistema.
struct ImportedMovement
{
    QDate date;
    QString concept;
    QString account;
    qint64 amountCents = 0;   // firmado tal cual viene en el fichero
    bool duplicate = false;   // ya existe en la base de datos
    QString error;            // no vacio si la fila es invalida
};

struct ImportResult
{
    int imported = 0;
    int skipped = 0;          // duplicados o desmarcados
    int failed = 0;           // no se pudieron crear
};

#endif // IMPORTTYPES_H
