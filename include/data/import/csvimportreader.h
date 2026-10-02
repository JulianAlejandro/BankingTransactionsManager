#ifndef CSVIMPORTREADER_H
#define CSVIMPORTREADER_H

#include "data/import/importreader.h"

class CsvImportReader : public ImportReader
{
public:
    bool read(const QString &path, QList<ImportedMovement> *out,
              QString *error = nullptr) const override;
};

#endif // CSVIMPORTREADER_H
