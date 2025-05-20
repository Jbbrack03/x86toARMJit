# Option to skip integer ops tests
option(SKIP_INTEGER_OPS_TESTS "Skip integer operations tests" OFF)

if(NOT SKIP_INTEGER_OPS_TESTS)
    add_executable(integer_ops_tests
        integer_ops_tests.cpp
    )

    target_link_libraries(integer_ops_tests
        PRIVATE
        xenoarm_jit
        gtest
        gtest_main
    )

    add_test(NAME integer_ops_tests COMMAND integer_ops_tests)
endif() 