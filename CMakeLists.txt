cmake_minimum_required(VERSION 3.16)

project(
    OrderTestingProject
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTS "Build unit tests" ON)

add_library(order_system
    src/Product.cpp
    src/Cart.cpp
    src/DiscountCalculator.cpp
    src/InventoryService.cpp
    src/PaymentService.cpp
    src/NotificationService.cpp
    src/Logger.cpp
    src/OrderService.cpp
)

target_include_directories(order_system
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

if(MSVC)
    target_compile_options(order_system PRIVATE /W4)
else()
    target_compile_options(
        order_system
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
endif()

add_executable(order_app
    src/main.cpp
)

target_link_libraries(
    order_app
    PRIVATE
        order_system
)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
