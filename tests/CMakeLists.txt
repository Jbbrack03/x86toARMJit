cmake_minimum_required(VERSION 3.10)

# Enable testing
enable_testing()

# Include GoogleTest for unit testing
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.13.0
)
FetchContent_MakeAvailable(googletest)

# Include directories
include_directories(
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/src
)

# Add test subdirectories based on skip options
if(NOT SKIP_CODE_GEN_TESTS)
  add_subdirectory(decoder)
endif()

if(NOT SKIP_CODE_GEN_TESTS)
  add_subdirectory(ir)
endif()

if(NOT SKIP_CODE_GEN_TESTS)
  add_subdirectory(code_gen)
endif()

if(NOT SKIP_DISPATCHER_TESTS)
  add_subdirectory(dispatcher)
endif()

if(NOT SKIP_INTEGER_OPS_TESTS)
  add_subdirectory(integer_ops)
endif()

if(NOT SKIP_CONTROL_FLOW_TESTS)
  add_subdirectory(control_flow)
endif()

if(NOT SKIP_SIMD_TESTS)
  add_subdirectory(simd)
endif()

# Phase 7: Add architectural tests and benchmarks
if(NOT SKIP_ARCH_TESTS)
  add_subdirectory(arch_tests)
endif()

if(NOT SKIP_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()

# Optional: Skip FPU exception test (it has linking issues)
if(NOT SKIP_FPU_TESTS)
  # FPU Exception Test
  add_executable(fpu_exception_test 
    fpu_exception_test.cpp
  )
  target_link_libraries(fpu_exception_test
    xenoarm_jit
    gtest_main
  )
  add_test(NAME fpu_exception_test COMMAND fpu_exception_test)
  
  # Phase 8: FPU Transcendental Functions Test
  add_executable(fpu_transcendental_test
    fpu_transcendental_test.cpp
  )
  target_link_libraries(fpu_transcendental_test
    xenoarm_jit
    gtest_main
  )
  add_test(NAME fpu_transcendental_test COMMAND fpu_transcendental_test)
  
  # Phase 8 Completion: FPU ARM Optimization Test
  add_executable(fpu_optimization_test
    fpu_optimization_test.cpp
  )
  target_link_libraries(fpu_optimization_test
    xenoarm_jit
    gtest_main
  )
  add_test(NAME fpu_optimization_test COMMAND fpu_optimization_test)
endif()

# Phase 8: Register Allocator with Spilling Support Test
add_executable(register_allocator_test
  register_allocator_test.cpp
)
target_link_libraries(register_allocator_test
  xenoarm_jit
  gtest_main
)
add_test(NAME register_allocator_test COMMAND register_allocator_test)

# Phase 6 Tests

# Skip memory model test (it requires significant IR changes)
if(NOT SKIP_MEMORY_MODEL_TEST)
  # Memory model test
  add_executable(memory_model_test
    memory_model_test.cpp
  )
  target_link_libraries(memory_model_test
    xenoarm_jit
    gtest_main
  )
  add_test(NAME memory_model_test COMMAND memory_model_test)
endif()

# SMC detection test
add_executable(smc_test 
  smc_test.cpp
)
target_link_libraries(smc_test
  xenoarm_jit
)
add_test(NAME smc_test COMMAND smc_test)

# API test - comprehensive testing of all API functions
add_executable(api_tests
  api_tests.cpp
)
target_link_libraries(api_tests
  xenoarm_jit
  gtest_main
)
add_test(NAME api_tests COMMAND api_tests) 