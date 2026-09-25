#include "model/transaction.h"

bool Transaction::isValid(QString *error) const
{
    if (!date.isValid()) {
        if (error) *error = QStringLiteral("La fecha no es valida.");
        return false;
    }
    if (concept.trimmed().isEmpty()) {
        if (error) *error = QStringLiteral("El concepto no puede estar vacio.");
        return false;
    }
    if (categoryId < 0) {
        if (error) *error = QStringLiteral("Debes seleccionar una categoria.");
        return false;
    }
    if (amountCents <= 0) {
        if (error) *error = QStringLiteral("El importe debe ser mayor que cero.");
        return false;
    }
    return true;
}
