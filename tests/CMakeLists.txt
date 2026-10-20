include(FetchContent)
FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)

add_executable(testes_lexico
    lexico/auxiliar_lexer.cpp
    lexico/tokens_validos_test.cpp)
target_link_libraries(testes_lexico PRIVATE portugolc_core GTest::gtest_main)
gtest_discover_tests(testes_lexico)