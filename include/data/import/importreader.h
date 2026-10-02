#ifndef IMPORTREADER_H
#define IMPORTREADER_H

#include <QList>
#include <QString>

#include "data/importtypes.h"

// Interfaz de los lectores de extractos (XLSX, CSV).
class ImportReader
{
public:
    virtual ~ImportReader() = default;

    // Lee el fichero y devuelve los movimientos encontrados. Los movimientos
    // con formato invalido se incluyen igualmente con 'error' informado.
    virtual bool read(const QString &path, QList<ImportedMovement> *out,
                      QString *error = nullptr) const = 0;
};

#endif // IMPORTREADER_H
