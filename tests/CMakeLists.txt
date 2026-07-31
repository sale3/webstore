find_package(GTest REQUIRED)

add_executable(order_system_tests
    SanityTest.cpp
    ProductInitialTest.cpp
)

target_include_directories(
    order_system_tests
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
    order_system_tests
    PRIVATE
        order_system
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
)

include(GoogleTest)

gtest_discover_tests(order_system_tests)
