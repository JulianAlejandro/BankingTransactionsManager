#include "data/import/importutils.h"

#include <QLocale>
#include <QRegularExpression>

namespace {

bool looksLikeMovementDescription(const QString &concept, const QString &movement)
{
    if (movement.isEmpty()) {
        return false;
    }
    if (movement.compare(concept, Qt::CaseInsensitive) == 0) {
        return false;
    }
    QString normalizedConcept = concept;
    QString normalizedMovement = movement;
    normalizedConcept.remove(QRegularExpression(QStringLiteral("[^A-Za-z0-9]")));
    normalizedMovement.remove(QRegularExpression(QStringLiteral("[^A-Za-z0-9]")));
    if (!normalizedMovement.isEmpty()
        && normalizedConcept.contains(normalizedMovement, Qt::CaseInsensitive)) {
        return false;
    }
    return true;
}

} // namespace

namespace ImportUtils {

QDate parseDate(const QString &value)
{
    const QString trimmed = value.trimmed();
    if (trimmed.isEmpty()) {
        return QDate();
    }

    const QDate iso = QDate::fromString(trimmed, Qt::ISODate);
    if (iso.isValid()) {
        return iso;
    }

    const QDate slash = QDate::fromString(trimmed, QStringLiteral("dd/MM/yyyy"));
    if (slash.isValid()) {
        return slash;
    }

    return QDate::fromString(trimmed, QStringLiteral("d/M/yyyy"));
}

qint64 parseAmountCents(const QString &value, bool *ok)
{
    QString cleaned = value.trimmed();
    cleaned.remove(QLatin1Char(' '));
    cleaned.remove(QStringLiteral("\u20ac"));
    if (cleaned.isEmpty()) {
        if (ok) *ok = false;
        return 0;
    }

    bool parsed = false;
    // El banco usa punto decimal; probamos C primero y es-ES despues.
    double number = QLocale::c().toDouble(cleaned, &parsed);
    if (!parsed) {
        number = QLocale(QLocale::Spanish, QLocale::Spain).toDouble(cleaned, &parsed);
    }
    if (!parsed) {
        if (ok) *ok = false;
        return 0;
    }

    if (ok) *ok = true;
    const double cents = number * 100.0;
    return static_cast<qint64>(cents >= 0.0 ? cents + 0.5 : cents - 0.5);
}

QString combineConcept(const QString &concept, const QString &movement)
{
    const QString c = concept.trimmed();
    const QString m = movement.trimmed();
    if (c.isEmpty()) {
        return m;
    }
    if (looksLikeMovementDescription(c, m)) {
        return c + QStringLiteral(" - ") + m;
    }
    return c;
}

} // namespace ImportUtils
