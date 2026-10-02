# BankingTransactionsManager

A desktop application to record personal bank movements and break each one down into
derived transactions classified in a hierarchical category tree, then explore them through
reports and charts.

## Features

- **Bank movements (parent transactions)**: date, concept, signed amount, account and notes.
  A movement has no category of its own; classification lives in its derived transactions.
- **Derived transactions**: every movement has at least one derived transaction. The strict
  rule is `sum(derived amounts) == parent amount`; each derived amount is signed according to
  its category (income positive, expense negative). A single unified editor handles the
  movement and its breakdown together.
- **Categories**: a hierarchical tree (adjacency list, unlimited depth) with two roots,
  *Gasto* (expense) and *Ingreso* (income). Derived transactions can be assigned to any node.
- **Reports**:
  - KPIs for a period: income, expenses, net and number of movements.
  - Breakdown by category, either for a single node or for its whole subtree
    ("include descendants" toggle), shown as a flat table with the full path.
  - Time evolution by day/week/month/quarter/year, with net and cumulative balance.
  - Period comparison (A vs B) with absolute and percentage variation.
  - Charts (Qt Charts): income/expense/net lines, cumulative balance and bars per category.
  - Drill-down: list the individual movements that make up a report row.
  - CSV export of the shown report table (Excel-friendly: `;` separator, `,` decimal, UTF-8 BOM).
- **Import** of bank statements from **XLSX** (BBVA "Informe" profile) and **CSV**, with
  duplicate detection (`date + concept + amount`) and a preview dialog before importing.

## Stack

- **Language**: C++17 (no compiler extensions)
- **Framework**: Qt 6 — Qt Widgets, Qt Sql (SQLite), Qt Charts
- **File import**: Qt private `QZipReader` (`Qt6::CorePrivate`) + `QXmlStreamReader` for XLSX
- **Build system**: CMake + Ninja
- **Kit used in development**: Desktop Qt 6.11.1 MinGW 64-bit (MinGW 13.1.0)

## Requirements

- Qt 6 with the Widgets, Sql and Charts modules
- A C++17 compiler (tested with MinGW 13.1.0)
- CMake and Ninja

## Build

```powershell
$env:Path = "C:\Qt\6.11.1\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\Ninja;" + $env:Path
& "C:\Qt\Tools\CMake_64\bin\cmake.exe" -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="C:\Qt\6.11.1\mingw_64" -DCMAKE_CXX_COMPILER="C:\Qt\Tools\mingw1310_64\bin\g++.exe"
& "C:\Qt\Tools\CMake_64\bin\cmake.exe" --build build
```

Run the application (Qt and MinGW must be on the `PATH`):

```powershell
.\build\GestionTransaccionesBancarias.exe
```

## Tests

```powershell
& "C:\Qt\Tools\CMake_64\bin\ctest.exe" --test-dir build --output-on-failure
```

## Seed example data

With the application closed:

```powershell
& "C:\Qt\Tools\CMake_64\bin\cmake.exe" --build build --target seed_demo
.\build\seed_demo.exe
```

## Project layout

```
include/ and src/
  appinfo.h            Application metadata
  model/               Entities and table models
  report/              Read-only report DTOs
  data/                SQLite persistence, services and statement readers
    import/            XLSX/CSV statement readers
  ui/                  Widgets (main window, dialogs, reports window)
tests/                 Qt Test suites
tools/                 Development tools (seed_demo)
```

The business logic lives in `data/` and is exposed to the UI through services and
repositories; the UI never writes SQL directly. `gestion_core` is a static library shared by
the application and the test suites.

## Data location

The SQLite database is stored outside the repository, in the platform's application data
directory (`QStandardPaths::AppDataLocation`), as `transacciones.db`.
