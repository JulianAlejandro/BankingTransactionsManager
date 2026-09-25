find_package(Qt6 REQUIRED COMPONENTS Test)

function(gestion_add_test target)
    qt_add_executable(${target} ${target}.cpp)
    target_link_libraries(${target} PRIVATE gestion_core Qt6::Test)
    add_test(NAME ${target} COMMAND ${target})
endfunction()

gestion_add_test(test_categoryrepository)
gestion_add_test(test_transactionrepository)
gestion_add_test(test_transactionservice)
