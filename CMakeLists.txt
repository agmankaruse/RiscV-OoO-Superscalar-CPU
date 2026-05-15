cmake_minimum_required(VERSION 3.16)

project(RiscV-OoO-Superscalar-CPU
    VERSION 0.1.0
    DESCRIPTION "Educational RV32I out-of-order superscalar CPU simulator"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(riscv_ooo_core
    src/branch_predictor.cpp
    src/cpu.cpp
    src/functional_unit.cpp
    src/isa.cpp
    src/issue_queue.cpp
    src/load_store_queue.cpp
    src/memory.cpp
    src/physical_register_file.cpp
    src/pipeline.cpp
    src/rename_table.cpp
    src/reorder_buffer.cpp
    src/stats.cpp
)

target_include_directories(riscv_ooo_core PUBLIC include)

if(MSVC)
    target_compile_options(riscv_ooo_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(riscv_ooo_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(riscv_ooo_sim src/main.cpp)
target_link_libraries(riscv_ooo_sim PRIVATE riscv_ooo_core)

enable_testing()

set(RISCV_OOO_TESTS
    test_arithmetic
    test_dependencies
    test_out_of_order
    test_load_store
    test_branch
    test_x0
)

foreach(test_name IN LISTS RISCV_OOO_TESTS)
    add_executable(${test_name} tests/${test_name}.cpp)
    target_link_libraries(${test_name} PRIVATE riscv_ooo_core)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
