#ifndef IMPORTDIALOG_H
#define IMPORTDIALOG_H

#include <QDialog>
#include <QList>

#include "data/importtypes.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableView;
class CategoryRepository;
class ImportService;
class ImportTableModel;

class ImportDialog : public QDialog
{
    Q_OBJECT

public:
    ImportDialog(ImportService *service, CategoryRepository *categories,
                 QWidget *parent = nullptr);

    int importedCount() const;

private slots:
    void browse();
    void loadFile(const QString &path);
    void updateSummary();
    void runImport();

private:
    void populateCategories();

    ImportService *m_service;
    CategoryRepository *m_categories;
    ImportTableModel *m_model;
    QList<ImportedMovement> m_movements;

    QLineEdit *m_path;
    QTableView *m_view;
    QComboBox *m_category;
    QCheckBox *m_selectAll;
    QLabel *m_summary;
    QPushButton *m_importButton;

    int m_imported = 0;
};

#endif // IMPORTDIALOG_H
