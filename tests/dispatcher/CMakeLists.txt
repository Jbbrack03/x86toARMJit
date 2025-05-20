# Option to skip dispatcher tests
option(SKIP_DISPATCHER_TESTS "Skip dispatcher tests" OFF)

if(NOT SKIP_DISPATCHER_TESTS)
    add_executable(dispatcher_tests
        dispatcher_tests.cpp
    )

    target_link_libraries(dispatcher_tests
        PRIVATE
        xenoarm_jit
        gtest
        gtest_main
    )

    add_test(NAME dispatcher_tests COMMAND dispatcher_tests)
endif() 