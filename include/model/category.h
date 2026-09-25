#ifndef CATEGORY_H
#define CATEGORY_H

#include <QString>

enum class CategoryType {
    Income,
    Expense
};

inline const char *categoryTypeToString(CategoryType type)
{
    return type == CategoryType::Income ? "ingreso" : "gasto";
}

inline CategoryType categoryTypeFromString(const QString &value)
{
    return value == QLatin1String("ingreso") ? CategoryType::Income : CategoryType::Expense;
}

struct Category
{
    int id = -1;
    int parentId = -1;
    QString name;
    CategoryType type = CategoryType::Expense;
    int sortOrder = 0;
    QString path;
    int depth = 0;
};

#endif // CATEGORY_H
