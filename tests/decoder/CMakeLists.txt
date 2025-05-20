add_executable(decoder_tests
    decoder_tests.cpp
)

target_link_libraries(decoder_tests
    PRIVATE
    xenoarm_jit
    gtest
    gtest_main
)

add_test(NAME decoder_tests COMMAND decoder_tests) 