#include "data/import/csvimportreader.h"

#include <QFile>
#include <QStringConverter>
#include <QStringList>
#include <QTextStream>

#include "data/import/importutils.h"

namespace {

QStringList splitLine(const QString &line, QChar separator)
{
    QStringList fields;
    QString current;
    bool inQuotes = false;
    for (int i = 0; i < line.size(); ++i) {
        const QChar ch = line.at(i);
        if (inQuotes) {
            if (ch == QLatin1Char('"')) {
                if (i + 1 < line.size() && line.at(i + 1) == QLatin1Char('"')) {
                    current.append(QLatin1Char('"'));
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                current.append(ch);
            }
        } else if (ch == QLatin1Char('"')) {
            inQuotes = true;
        } else if (ch == separator) {
            fields.append(current);
            current.clear();
        } else {
            current.append(ch);
        }
    }
    fields.append(current);
    return fields;
}

QChar detectSeparator(const QString &headerLine)
{
    const QStringList candidates = {QStringLiteral(";"), QStringLiteral(","),
                                    QStringLiteral("\t")};
    QChar best = QLatin1Char(';');
    int bestCount = -1;
    for (const QString &candidate : candidates) {
        const int count = headerLine.count(candidate);
        if (count > bestCount) {
            bestCount = count;
            best = candidate.at(0);
        }
    }
    return best;
}

} // namespace

bool CsvImportReader::read(const QString &path, QList<ImportedMovement> *out,
                           QString *error) const
{
    if (out == nullptr) {
        if (error) *error = QStringLiteral("La salida no puede ser nula.");
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error) *error = QStringLiteral("No se pudo abrir el fichero.");
        return false;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);

    out->clear();
    QString headerLine;
    if (!stream.readLineInto(&headerLine)) {
        if (error) *error = QStringLiteral("El fichero esta vacio.");
        return false;
    }
    headerLine.remove(QChar(0xFEFF));

    const QChar separator = detectSeparator(headerLine);
    const QStringList headers = splitLine(headerLine, separator);

    int dateColumn = -1;
    int conceptColumn = -1;
    int amountColumn = -1;
    int movementColumn = -1;
    for (int i = 0; i < headers.size(); ++i) {
        const QString name = headers.at(i).trimmed().toLower();
        if (dateColumn < 0 && (name.contains(QStringLiteral("fecha")))) {
            dateColumn = i;
        } else if (amountColumn < 0 && name.contains(QStringLiteral("importe"))) {
            amountColumn = i;
        } else if (conceptColumn < 0 && name.contains(QStringLiteral("concepto"))) {
            conceptColumn = i;
        } else if (movementColumn < 0 && name.contains(QStringLiteral("movimiento"))) {
            movementColumn = i;
        }
    }

    if (dateColumn < 0 || amountColumn < 0) {
        if (error) *error = QStringLiteral("No se encontraron las columnas Fecha/Importe.");
        return false;
    }

    QString line;
    while (stream.readLineInto(&line)) {
        if (line.trimmed().isEmpty()) {
            continue;
        }
        const QStringList fields = splitLine(line, separator);
        const auto field = [&fields](int index) -> QString {
            return index >= 0 && index < fields.size() ? fields.at(index) : QString();
        };

        ImportedMovement movement;
        movement.date = ImportUtils::parseDate(field(dateColumn));
        const QString concept = field(conceptColumn);
        movement.concept = ImportUtils::combineConcept(concept, field(movementColumn));

        bool amountOk = false;
        movement.amountCents = ImportUtils::parseAmountCents(field(amountColumn), &amountOk);

        if (!movement.date.isValid()) {
            movement.error = QStringLiteral("Fecha invalida.");
        } else if (!amountOk) {
            movement.error = QStringLiteral("Importe invalido.");
        } else if (movement.concept.trimmed().isEmpty()) {
            movement.error = QStringLiteral("Concepto vacio.");
        }

        out->append(movement);
    }

    return true;
}
