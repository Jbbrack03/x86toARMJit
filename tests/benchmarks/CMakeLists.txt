# CMakeLists.txt for XenoARM JIT benchmarks

# Add the executable
add_executable(benchmark_runner benchmark_runner.cpp)

# Link against the JIT library and other dependencies
target_link_libraries(benchmark_runner
    xenoarm_jit
)

# Add a custom target to run benchmarks (not as a test, as it takes time)
add_custom_target(run_benchmarks
    COMMAND benchmark_runner
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS benchmark_runner
    COMMENT "Running XenoARM JIT benchmarks"
) 