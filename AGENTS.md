# AGENTS.md

Este documento describe el proyecto y sus convenciones. Es la fuente de verdad para
humanos y agentes que trabajen en el repositorio. **Debe mantenerse actualizado**: al
anadir una caracteristica o cambiar una decision de diseno, se refleja aqui.

## Proyecto

- **Nombre**: GestionTransaccionesBancarias
- **Version**: 0.1.0
- **Organizacion**: Julale
- **Descripcion**: Aplicacion de escritorio para registrar movimientos bancarios
  personales y desglosarlos en transacciones derivadas clasificadas en un arbol de
  categorias.

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
  model/               Entidades puras + modelos de tabla
    transaction.*        Movimiento bancario (padre); importe firmado
    derivedtransaction.* Transaccion derivada; importe firmado segun categoria
    category.*           Entidad Category + CategoryType
    transactiontablemodel.*  Modelo de la tabla principal
    derivedtablemodel.*      Modelo de la tabla del desglose
  data/                Persistencia y logica de agregado
    database.*           Apertura SQLite, esquema versionado y seed
    categoryrepository.* Repositorio del arbol de categorias
    transactionrepository.*  CRUD del movimiento bancario
    derivedtransactionrepository.* CRUD de las derivadas
    transactionservice.*  Coordina "transaccion + derivadas" de forma atomica
  ui/                  Interfaz
    mainwindow.*              Ventana principal (tabla + toolbar)
    transactioneditordialog.* Editor unificado (movimiento + desglose)
    derivedrowdialog.*        Alta/edicion de una derivada
tests/                 Tests con Qt Test
```

- `gestion_core` es una libreria estatica con `model/`, `data/` y los modelos de tabla.
  La enlazan tanto la aplicacion como los tests.
- La aplicacion enlaza `gestion_core` + `Qt6::Widgets`.
- La UI solo habla con `TransactionService` y `CategoryRepository`; nunca escribe SQL.

## Convenciones

- **C++17**, sin extensiones (`CMAKE_CXX_EXTENSIONS OFF`).
- **Idioma del codigo**: identificadores en ingles; comentarios y textos de UI en espanol.
- **Importes**: se almacenan como `qint64` en **centimos con signo** (ingreso `> 0`,
  gasto `< 0`); nunca `double` en persistencia. El formateo se hace con
  `QLocale(QLocale::Spanish, QLocale::Spain)` (EUR).
- **Signo de una derivada**: lo determina su categoria (`signedAmountFor`).
- **Fechas**: se guardan como texto ISO (`Qt::ISODate`) en SQLite.
- **Errores**: repositorios y servicio devuelven `bool` y escriben el mensaje en `QString *error`.
- **Commits**: convencion `tipo(ambito): descripcion` (p. ej. `feat(data): ...`).
  Un commit por fase/capa.
- No anadir dependencias nuevas sin acordarlo.

## Especificacion funcional

### Movimiento bancario (transaccion padre)

- Es una copia de la operacion tal como la da el banco.
- Campos: `fecha`, `concepto`, `importe` (firmado), `cuenta`, `notas`. **No tiene categoria**:
  la clasificacion vive en sus derivadas.
- Validacion: fecha valida, concepto no vacio, importe distinto de cero.

### Transacciones derivadas

- Toda transaccion tiene **al menos una** derivada; al crear una transaccion se crea una
  derivada por defecto que hereda concepto, notas e importe del padre.
- Campos: `transaction_id`, `category_id` (obligatoria), `concepto` (si vacio hereda el del
  padre), `importe` (firmado segun la categoria), `notas`.
- **Coherencia estricta**: `sum(importe de las derivadas) == importe del padre`. El editor
  solo permite guardar cuando cuadra.
- Se editan junto al padre en un **editor unico** (movimiento arriba, desglose abajo).
- El dialogo de una derivada ofrece **"Cargar del padre"**: vuelca concepto, notas e importe
  (magnitud) del movimiento; la categoria la elige el usuario y determina el signo. Si la
  categoria elegida no coincide de signo con el padre, se avisa.
- La ventana principal muestra un **panel inferior** que, al seleccionar un movimiento,
  lista sus derivadas (solo lectura) y su total.
- No se puede eliminar la ultima derivada de una transaccion.
- Al borrar el padre se borran sus derivadas (`ON DELETE CASCADE`).

### Categorias

- Estructura de **arbol jerarquico** almacenada como **lista de adyacencia**
  (`categories.parent_id`; `NULL` para raices). Profundidad ilimitada.
- Dos raices: **Gasto** e **Ingreso**. Cada categoria guarda su `type` (`ingreso`/`gasto`)
  denormalizado; los hijos heredan el tipo de su raiz.
- Las derivadas pueden asignarse a **cualquier nodo** (hoja o intermedio).
- Consultas recursivas (`WITH RECURSIVE`) calculan subarboles y la ruta
  (`Gasto > Comida > Restaurantes`).
- Integridad: `ON DELETE RESTRICT`; no se puede borrar una categoria con hijos ni usada
  por derivadas. No se permite mover una categoria dentro de su propio subarbol ni entre
  ramas de distinto tipo.

### Esquema SQLite (version 2)

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
    importe INTEGER NOT NULL,          -- centimos con signo
    cuenta TEXT,
    notas TEXT,
    creado_en TEXT NOT NULL
);

CREATE TABLE derived_transactions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    transaction_id INTEGER NOT NULL REFERENCES transactions(id) ON DELETE CASCADE,
    category_id INTEGER NOT NULL REFERENCES categories(id) ON DELETE RESTRICT,
    concepto TEXT,
    importe INTEGER NOT NULL,          -- centimos, signo segun la categoria
    notas TEXT,
    creado_en TEXT NOT NULL
);
```

- `PRAGMA user_version = 2`. Si al abrir la version almacenada es inferior, se **recrean**
  las tablas (reset de desarrollo, aun sin datos reales). Cuando haya datos habra que
  sustituirlo por migraciones reales.
- La base de datos vive en `QStandardPaths::AppDataLocation` (`transacciones.db`).

## Roadmap

Pendiente (fuera del MVP actual):

- Gestor de categorias en la UI (crear, renombrar, mover, borrar).
- Informes: totales por **nodo** de categoria y por **rama** (subarbol, con `WITH RECURSIVE`),
  basados en las transacciones derivadas.
- Resumen y graficas (ingresos/gastos/saldo) con Qt Charts.
- Filtros y busqueda en la tabla (por fecha, categoria, texto).
- Internacionalizacion (`.ts`) e icono de aplicacion.
- Empaquetado/deploy con `windeployqt`.
