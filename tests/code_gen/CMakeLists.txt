# Option to skip code generator tests
option(SKIP_CODE_GEN_TESTS "Skip code generator tests" OFF)

if(NOT SKIP_CODE_GEN_TESTS)
    add_executable(code_gen_tests
        code_gen_tests.cpp
    )

    target_link_libraries(code_gen_tests
        PRIVATE
        xenoarm_jit
        gtest
        gtest_main
    )

    add_test(NAME code_gen_tests COMMAND code_gen_tests)
endif() 