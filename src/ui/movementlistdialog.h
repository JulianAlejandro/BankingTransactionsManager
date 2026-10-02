#ifndef MOVEMENTLISTDIALOG_H
#define MOVEMENTLISTDIALOG_H

#include <QDialog>
#include <QList>

#include "report/reporttypes.h"

class QLabel;
class QTableView;
class MovementTableModel;

class MovementListDialog : public QDialog
{
    Q_OBJECT

public:
    MovementListDialog(const QString &title, const QList<MovementRow> &rows,
                       QWidget *parent = nullptr);

private:
    MovementTableModel *m_model;
    QTableView *m_view;
};

#endif // MOVEMENTLISTDIALOG_H
