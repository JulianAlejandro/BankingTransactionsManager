#include "model/derivedtransaction.h"

bool DerivedTransaction::isValid(QString *error) const
{
    if (categoryId < 0) {
        if (error) *error = QStringLiteral("Debes seleccionar una categoria.");
        return false;
    }
    if (amountCents == 0) {
        if (error) *error = QStringLiteral("El importe debe ser distinto de cero.");
        return false;
    }
    return true;
}
