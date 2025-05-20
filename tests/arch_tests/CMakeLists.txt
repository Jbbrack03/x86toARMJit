# CMakeLists.txt for XenoARM JIT architectural tests

# Add the executable
add_executable(arch_test_runner arch_test_runner.cpp)

# Explicitly set include directories
target_include_directories(arch_test_runner PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/logging
    ${CMAKE_SOURCE_DIR}/src/jit_core
)

# Link against the JIT library and other dependencies
target_link_libraries(arch_test_runner
    xenoarm_jit
)

# Add the test
add_test(
    NAME ArchitecturalTests
    COMMAND arch_test_runner
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
) 