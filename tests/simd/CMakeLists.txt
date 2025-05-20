# Option to skip SIMD tests
option(SKIP_SIMD_TESTS "Skip SIMD tests" OFF)

if(NOT SKIP_SIMD_TESTS)
    add_executable(simd_tests
        simd_tests.cpp
    )

    target_link_libraries(simd_tests
        PRIVATE
        xenoarm_jit
        gtest
        gtest_main
    )

    add_test(NAME simd_tests COMMAND simd_tests)
endif() 