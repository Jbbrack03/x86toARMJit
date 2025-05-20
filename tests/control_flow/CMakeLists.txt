# Option to skip control flow tests
option(SKIP_CONTROL_FLOW_TESTS "Skip control flow tests" OFF)

if(NOT SKIP_CONTROL_FLOW_TESTS)
    add_executable(control_flow_tests
        control_flow_tests.cpp
    )

    target_link_libraries(control_flow_tests
        PRIVATE
        xenoarm_jit
        gtest
        gtest_main
    )

    add_test(NAME control_flow_tests COMMAND control_flow_tests)
endif() 