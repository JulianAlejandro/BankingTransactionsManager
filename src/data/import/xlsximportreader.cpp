#include "data/import/xlsximportreader.h"

#include <QHash>
#include <QXmlStreamReader>

#include "data/import/importutils.h"

#include <private/qzipreader_p.h>

namespace {

// Extrae el texto de un <si> de sharedStrings.xml.
QString readSharedString(QXmlStreamReader &xml)
{
    QString text;
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isEndElement() && xml.name() == QLatin1String("si")) {
            break;
        }
        if (xml.isStartElement() && xml.name() == QLatin1String("t")) {
            text += xml.readElementText();
        }
    }
    return text;
}

// "D6" -> "D".
QString columnFromReference(const QString &reference)
{
    QString column;
    for (const QChar ch : reference) {
        if (ch.isLetter()) {
            column.append(ch.toUpper());
        } else {
            break;
        }
    }
    return column;
}

} // namespace

bool XlsxImportReader::read(const QString &path, QList<ImportedMovement> *out,
                            QString *error) const
{
    if (out == nullptr) {
        if (error) *error = QStringLiteral("La salida no puede ser nula.");
        return false;
    }

    QZipReader zip(path);
    if (!zip.isReadable()) {
        if (error) *error = QStringLiteral("No se pudo abrir el fichero XLSX.");
        return false;
    }

    // 1) Tabla de cadenas compartidas.
    QStringList sharedStrings;
    const QByteArray sharedData = zip.fileData(QStringLiteral("xl/sharedStrings.xml"));
    if (!sharedData.isEmpty()) {
        QXmlStreamReader xml(sharedData);
        while (!xml.atEnd()) {
            xml.readNext();
            if (xml.isStartElement() && xml.name() == QLatin1String("si")) {
                sharedStrings.append(readSharedString(xml));
            }
        }
    }

    const QByteArray sheetData = zip.fileData(QStringLiteral("xl/worksheets/sheet1.xml"));
    if (sheetData.isEmpty()) {
        if (error) *error = QStringLiteral("El XLSX no tiene hoja de calculo.");
        return false;
    }

    out->clear();

    QHash<QString, QString> cells;   // columna -> valor de la fila en curso
    QString pendingColumn;
    QString pendingType;
    QString pendingValue;

    QString dateColumn;
    QString conceptColumn;
    QString movementColumn;
    QString amountColumn;
    bool headerFound = false;

    const auto resolve = [&](const QString &type, const QString &value) -> QString {
        if (type == QLatin1String("s")) {
            bool ok = false;
            const int index = value.toInt(&ok);
            if (ok && index >= 0 && index < sharedStrings.size()) {
                return sharedStrings.at(index);
            }
            return QString();
        }
        return value;
    };

    const auto processRow = [&]() {
        if (!headerFound) {
            for (auto it = cells.constBegin(); it != cells.constEnd(); ++it) {
                const QString name = it.value().trimmed().toLower();
                if (dateColumn.isEmpty() && name == QLatin1String("fecha")) {
                    dateColumn = it.key();
                } else if (amountColumn.isEmpty() && name == QLatin1String("importe")) {
                    amountColumn = it.key();
                } else if (conceptColumn.isEmpty() && name == QLatin1String("concepto")) {
                    conceptColumn = it.key();
                } else if (movementColumn.isEmpty() && name == QLatin1String("movimiento")) {
                    movementColumn = it.key();
                }
            }
            if (!dateColumn.isEmpty() && !amountColumn.isEmpty() && !conceptColumn.isEmpty()) {
                headerFound = true;
            }
            return;
        }

        const QString concept = cells.value(conceptColumn);
        const QString amount = cells.value(amountColumn);
        const QString date = cells.value(dateColumn);
        if (date.isEmpty() && concept.isEmpty() && amount.isEmpty()) {
            return; // fila vacia o decorativa
        }

        ImportedMovement movement;
        movement.date = ImportUtils::parseDate(date);
        movement.concept = ImportUtils::combineConcept(concept, cells.value(movementColumn));

        bool amountOk = false;
        movement.amountCents = ImportUtils::parseAmountCents(amount, &amountOk);

        if (!movement.date.isValid()) {
            movement.error = QStringLiteral("Fecha invalida.");
        } else if (!amountOk) {
            movement.error = QStringLiteral("Importe invalido.");
        } else if (movement.concept.trimmed().isEmpty()) {
            movement.error = QStringLiteral("Concepto vacio.");
        }
        out->append(movement);
    };

    QXmlStreamReader xml(sheetData);
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement() && xml.name() == QLatin1String("row")) {
            cells.clear();
        } else if (xml.isStartElement() && xml.name() == QLatin1String("c")) {
            pendingColumn = columnFromReference(
                xml.attributes().value(QStringLiteral("r")).toString());
            pendingType = xml.attributes().value(QStringLiteral("t")).toString();
            pendingValue.clear();
        } else if (xml.isStartElement() && xml.name() == QLatin1String("v")) {
            pendingValue = xml.readElementText();
        } else if (xml.isEndElement() && xml.name() == QLatin1String("c")) {
            cells.insert(pendingColumn, resolve(pendingType, pendingValue));
        } else if (xml.isEndElement() && xml.name() == QLatin1String("row")) {
            processRow();
        }
    }

    if (!headerFound) {
        if (error) *error = QStringLiteral("No se encontro la fila de cabecera.");
        return false;
    }

    return true;
}
