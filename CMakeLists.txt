cmake_minimum_required(VERSION 3.16)

project(RiscV-OoO-Superscalar-CPU
    VERSION 0.1.0
    DESCRIPTION "Educational RV32I out-of-order superscalar CPU simulator"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_WARNINGS "Enable compiler warnings" ON)
option(ENABLE_ASAN "Enable AddressSanitizer on supported compilers" OFF)
option(ENABLE_TRACE "Enable trace-oriented compile definitions" OFF)
option(ENABLE_TESTS "Build and register tests" ON)

add_library(riscv_ooo_core
    src/branch_predictor.cpp
    src/cache.cpp
    src/config.cpp
    src/cpu.cpp
    src/functional_unit.cpp
    src/invariant_checker.cpp
    src/isa.cpp
    src/issue_queue.cpp
    src/load_store_queue.cpp
    src/memory.cpp
    src/physical_register_file.cpp
    src/pipeline.cpp
    src/reference_cpu.cpp
    src/rename_table.cpp
    src/reorder_buffer.cpp
    src/stats.cpp
)

target_include_directories(riscv_ooo_core PUBLIC include)

if(ENABLE_WARNINGS)
    if(MSVC)
        target_compile_options(riscv_ooo_core PRIVATE /W4 /permissive-)
    else()
        target_compile_options(riscv_ooo_core PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

if(ENABLE_ASAN AND NOT MSVC)
    target_compile_options(riscv_ooo_core PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(riscv_ooo_core PUBLIC -fsanitize=address)
endif()

if(ENABLE_TRACE)
    target_compile_definitions(riscv_ooo_core PUBLIC RISCV_OOO_ENABLE_TRACE=1)
endif()

add_executable(riscv_ooo_sim src/main.cpp)
target_link_libraries(riscv_ooo_sim PRIVATE riscv_ooo_core)

if(ENABLE_TESTS)
    enable_testing()

    set(RISCV_OOO_TESTS
        test_arithmetic
        test_dependencies
        test_out_of_order
        test_load_store
        test_branch
        test_branch_predictor
        test_cache
        test_config
        test_x0
        test_rv32m
        test_timeline
        test_reference_cpu
        test_differential
        test_invariants
        test_random_programs
        test_branch_flush_correctness
        test_precise_commit
        test_memory_ordering
    )

    foreach(test_name IN LISTS RISCV_OOO_TESTS)
        add_executable(${test_name} tests/${test_name}.cpp)
        target_link_libraries(${test_name} PRIVATE riscv_ooo_core)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
endif()
