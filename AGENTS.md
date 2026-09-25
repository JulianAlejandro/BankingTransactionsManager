# AGENTS.md

Este documento describe el proyecto y sus convenciones. Es la fuente de verdad para
humanos y agentes que trabajen en el repositorio. **Debe mantenerse actualizado**: al
añadir una característica o cambiar una decision de diseno, se refleja aqui.

## Proyecto

- **Nombre**: GestionTransaccionesBancarias
- **Version**: 0.1.0
- **Organizacion**: Julale
- **Descripcion**: Aplicacion de escritorio para registrar y gestionar transacciones
  bancarias personales (ingresos y gastos) clasificadas en un arbol de categorias.

## Stack

- **Lenguaje**: C++17
- **Framework**: Qt 6 (Qt Widgets)
- **Modulo de datos**: Qt Sql (SQLite)
- **Build system**: CMake + Ninja
- **Kit**: Desktop Qt 6.11.1 MinGW 64-bit (MinGW 13.1.0)
- **Control de versiones**: Git (rama `main`)

## Comandos

Configurar y compilar (Debug):

```powershell
$env:Path = "C:\Qt\6.11.1\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\Ninja;" + $env:Path
& "C:\Qt\Tools\CMake_64\bin\cmake.exe" -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="C:\Qt\6.11.1\mingw_64" -DCMAKE_CXX_COMPILER="C:\Qt\Tools\mingw1310_64\bin\g++.exe"
& "C:\Qt\Tools\CMake_64\bin\cmake.exe" --build build
```

Ejecutar la aplicacion (requiere Qt y MinGW en el `PATH`):

```powershell
.\build\GestionTransaccionesBancarias.exe
```

Ejecutar los tests:

```powershell
& "C:\Qt\Tools\CMake_64\bin\ctest.exe" --test-dir build --output-on-failure
```

En Qt Creator: abrir `CMakeLists.txt`, seleccionar el kit MinGW 64-bit y usar Ctrl+R.

## Arquitectura

Separacion en capas; la logica no vive en la UI.

```
include/  y  src/
  appinfo.h            Metadatos de la aplicacion (nombre, version, organizacion)
  model/               Entidades puras + modelo de tabla
    transaction.*        Entidad Transaction (importe en centimos)
    category.*           Entidad Category + CategoryType
    transactiontablemodel.*  QAbstractTableModel para la vista
  data/                Persistencia
    database.*           Apertura SQLite, esquema y seed
    categoryrepository.* Repositorio del arbol de categorias
    transactionrepository.* Repositorio de transacciones
  ui/                  Interfaz
    mainwindow.*         Ventana principal (tabla + toolbar)
    transactiondialog.*  Dialogo modal de alta/edicion
tests/                 Tests con Qt Test
```

- `gestion_core` es una libreria estatica con `model/`, `data/` y `transactiontablemodel`.
  La enlazan tanto la aplicacion como los tests.
- La aplicacion enlaza `gestion_core` + `Qt6::Widgets`.

## Convenciones

- **C++17**, sin extensiones (`CMAKE_CXX_EXTENSIONS OFF`).
- **Idioma del codigo**: identificadores en ingles; comentarios y textos de UI en espanol.
- **Importes**: se almacenan como `qint64` en **centimos** (nunca `double` en persistencia).
  El formateo a moneda se hace con `QLocale(QLocale::Spanish, QLocale::Spain)` (EUR).
- **Fechas**: se guardan como texto ISO (`Qt::ISODate`) en SQLite.
- **Errores**: los repositorios devuelven `bool` y escriben el mensaje en `QString *error`.
- **Commits**: convencion `tipo(ambito): descripcion` (p. ej. `feat(data): ...`).
  Un commit por fase/capa.
- No anadir dependencias nuevas sin acordarlo.

## Especificacion funcional

### Transacciones

- Tipos: **ingreso** y **gasto**. El tipo **no** se guarda en la transaccion: se deduce
  de la categoria asociada.
- Campos: `fecha`, `concepto`, `category_id`, `importe` (centimos), `cuenta`, `notas`.
- Se crean y editan mediante un dialogo modal; la tabla es de solo lectura.
- Validacion: fecha valida, concepto no vacio, categoria seleccionada, importe > 0.

### Categorias

- Estructura de **arbol jerarquico** almacenada en SQL como **lista de adyacencia**
  (`categories.parent_id`; `NULL` para raices). Profundidad ilimitada.
- Dos raices: **Gasto** e **Ingreso**. Cada categoria guarda su `type` (`ingreso`/`gasto`)
  denormalizado; los hijos heredan el tipo de su raiz.
- Las transacciones pueden asignarse a **cualquier nodo** (hoja o intermedio).
- Consultas recursivas (`WITH RECURSIVE`) calculan subarboles y la ruta
  (`Gasto > Comida > Restaurantes`).
- Integridad: `ON DELETE RESTRICT`; no se puede borrar una categoria con hijos ni usada
  por transacciones. No se permite mover una categoria dentro de su propio subarbol ni
  entre ramas de distinto tipo.

### Esquema SQLite

```sql
CREATE TABLE categories (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    parent_id INTEGER REFERENCES categories(id) ON DELETE RESTRICT,
    name TEXT NOT NULL,
    type TEXT NOT NULL CHECK (type IN ('ingreso','gasto')),
    sort_order INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE transactions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    fecha TEXT NOT NULL,
    concepto TEXT NOT NULL,
    category_id INTEGER NOT NULL REFERENCES categories(id) ON DELETE RESTRICT,
    importe INTEGER NOT NULL,
    cuenta TEXT,
    notas TEXT,
    creado_en TEXT NOT NULL
);
```

La base de datos vive en `QStandardPaths::AppDataLocation` (`transacciones.db`).

## Roadmap

Pendiente (fuera del MVP actual):

- Gestor de categorias en la UI (crear, renombrar, mover, borrar).
- Resumen y graficas (totales ingresos/gastos/saldo, por categoria) con Qt Charts.
- Filtros y busqueda en la tabla (por fecha, categoria, texto).
- Internacionalizacion (`.ts`) e icono de aplicacion.
- Empaquetado/deploy con `windeployqt`.
