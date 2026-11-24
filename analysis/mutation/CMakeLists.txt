#  biblioteka - da je mogu koristiti i alat i testovi
add_library(mutation_core mutation_core.cpp)
target_include_directories(mutation_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#  alat
add_executable(mutation_tester mutation_testing.cpp)
target_link_libraries(mutation_tester PRIVATE mutation_core)
target_compile_definitions(mutation_tester PRIVATE
    PROJECT_ROOT="${PROJECT_SOURCE_DIR}"
    DEFAULT_BUILD_DIR="${CMAKE_BINARY_DIR}"
)
set_target_properties(mutation_tester PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# testovi alata 
if(BUILD_TESTS AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt)
    add_subdirectory(tests)
endif()