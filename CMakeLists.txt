cmake_minimum_required(VERSION 3.16)

project(GestionTransaccionesBancarias
    VERSION 0.1.0
    DESCRIPTION "Gestion de transacciones bancarias"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql Gui Charts CorePrivate)

qt_standard_project_setup()

qt_add_library(gestion_core STATIC
    include/model/category.h
    include/model/transaction.h
    include/model/derivedtransaction.h
    include/model/transactiontablemodel.h
    include/model/derivedtablemodel.h
    include/model/reporttablemodel.h
    include/model/timeseriesmodel.h
    include/model/movementtablemodel.h
    include/model/comparisontablemodel.h
    include/model/importtablemodel.h
    include/data/database.h
    include/data/categoryrepository.h
    include/data/transactionrepository.h
    include/data/derivedtransactionrepository.h
    include/data/transactionservice.h
    include/data/reportrepository.h
    include/data/importservice.h
    include/data/importtypes.h
    include/data/import/importreader.h
    include/data/import/importutils.h
    include/data/import/csvimportreader.h
    include/data/import/xlsximportreader.h
    include/report/reporttypes.h
    src/model/category.cpp
    src/model/transaction.cpp
    src/model/derivedtransaction.cpp
    src/model/transactiontablemodel.cpp
    src/model/derivedtablemodel.cpp
    src/model/reporttablemodel.cpp
    src/model/timeseriesmodel.cpp
    src/model/movementtablemodel.cpp
    src/model/comparisontablemodel.cpp
    src/model/importtablemodel.cpp
    src/data/database.cpp
    src/data/categoryrepository.cpp
    src/data/transactionrepository.cpp
    src/data/derivedtransactionrepository.cpp
    src/data/transactionservice.cpp
    src/data/reportrepository.cpp
    src/data/importservice.cpp
    src/data/import/importutils.cpp
    src/data/import/csvimportreader.cpp
    src/data/import/xlsximportreader.cpp
)

target_include_directories(gestion_core PUBLIC include)
target_link_libraries(gestion_core
    PUBLIC Qt6::Core Qt6::Sql Qt6::Gui
    PRIVATE Qt6::CorePrivate
)

qt_add_executable(GestionTransaccionesBancarias
    src/main.cpp
    src/ui/mainwindow.cpp
    src/ui/mainwindow.h
    src/ui/mainwindow.ui
    src/ui/transactioneditordialog.cpp
    src/ui/transactioneditordialog.h
    src/ui/transactioneditordialog.ui
    src/ui/derivedrowdialog.cpp
    src/ui/derivedrowdialog.h
    src/ui/derivedrowdialog.ui
    src/ui/reportfilterwidget.cpp
    src/ui/reportfilterwidget.h
    src/ui/reportwindow.cpp
    src/ui/reportwindow.h
    src/ui/movementlistdialog.cpp
    src/ui/movementlistdialog.h
    src/ui/importdialog.cpp
    src/ui/importdialog.h
    resources/resources.qrc
)

target_compile_definitions(GestionTransaccionesBancarias PRIVATE
    APP_VERSION="${PROJECT_VERSION}"
)

target_include_directories(GestionTransaccionesBancarias PRIVATE src)

target_link_libraries(GestionTransaccionesBancarias PRIVATE Qt6::Widgets Qt6::Charts gestion_core)

set_target_properties(GestionTransaccionesBancarias PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)

include(CTest)
if(BUILD_TESTING AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
endif()

qt_add_executable(seed_demo
    tools/seed_demo.cpp
)

set_target_properties(seed_demo PROPERTIES
    WIN32_EXECUTABLE OFF
)

target_link_libraries(seed_demo PRIVATE gestion_core)
