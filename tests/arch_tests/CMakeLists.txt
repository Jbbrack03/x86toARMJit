# CMakeLists.txt for XenoARM JIT architectural tests

# Add the executable
add_executable(arch_test_runner arch_test_runner.cpp)

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