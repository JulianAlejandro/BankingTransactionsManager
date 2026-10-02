#ifndef IMPORTUTILS_H
#define IMPORTUTILS_H

#include <QDate>
#include <QString>

namespace ImportUtils {

// Fecha en formato dd/MM/yyyy (texto de la celda Excel del banco).
QDate parseDate(const QString &value);

// Importe en euros (punto decimal) a centimos. 'ok' indica si se pudo parsear.
qint64 parseAmountCents(const QString &value, bool *ok = nullptr);

// Concepto combinado: "Concepto - Movimiento" si el movimiento aporta.
QString combineConcept(const QString &concept, const QString &movement);

} // namespace ImportUtils

#endif // IMPORTUTILS_H
