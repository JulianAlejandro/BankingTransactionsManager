#ifndef XLSXIMPORTREADER_H
#define XLSXIMPORTREADER_H

#include "data/import/importreader.h"

// Lee el "Informe BBVA" (una hoja, cabecera detectada por contenido).
class XlsxImportReader : public ImportReader
{
public:
    bool read(const QString &path, QList<ImportedMovement> *out,
              QString *error = nullptr) const override;
};

#endif // XLSXIMPORTREADER_H
