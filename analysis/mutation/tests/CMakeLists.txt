# Testovi alata za mutaciono testiranje.

find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(mutation_core_tests
    MutationCoreTest.cpp
    CommandRunnerTest.cpp
)

target_include_directories(mutation_core_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(mutation_core_tests PRIVATE
    mutation_core
    GTest::gtest
    GTest::gmock
    GTest::gtest_main
)

# Prefiks da ih mutation_tester može preskočiti (ctest -E "^MutationTool\.")
gtest_discover_tests(mutation_core_tests TEST_PREFIX "MutationTool.")