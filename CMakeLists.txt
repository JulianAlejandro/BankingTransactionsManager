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

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql Gui)

qt_standard_project_setup()

qt_add_library(gestion_core STATIC
    include/model/category.h
    include/model/transaction.h
    include/model/transactiontablemodel.h
    include/data/database.h
    include/data/categoryrepository.h
    include/data/transactionrepository.h
    src/model/category.cpp
    src/model/transaction.cpp
    src/model/transactiontablemodel.cpp
    src/data/database.cpp
    src/data/categoryrepository.cpp
    src/data/transactionrepository.cpp
)

target_include_directories(gestion_core PUBLIC include)
target_link_libraries(gestion_core PUBLIC Qt6::Core Qt6::Sql Qt6::Gui)

qt_add_executable(GestionTransaccionesBancarias
    src/main.cpp
    src/ui/mainwindow.cpp
    src/ui/mainwindow.h
    src/ui/mainwindow.ui
    src/ui/transactiondialog.cpp
    src/ui/transactiondialog.h
    src/ui/transactiondialog.ui
    resources/resources.qrc
)

target_compile_definitions(GestionTransaccionesBancarias PRIVATE
    APP_VERSION="${PROJECT_VERSION}"
)

target_include_directories(GestionTransaccionesBancarias PRIVATE src)

target_link_libraries(GestionTransaccionesBancarias PRIVATE Qt6::Widgets gestion_core)

set_target_properties(GestionTransaccionesBancarias PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)

include(CTest)
if(BUILD_TESTING AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
endif()
