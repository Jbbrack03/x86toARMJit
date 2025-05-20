add_executable(ir_tests
    ir_tests.cpp
)

target_link_libraries(ir_tests
    PRIVATE
    xenoarm_jit
    gtest
    gtest_main
)

add_test(NAME ir_tests COMMAND ir_tests) 