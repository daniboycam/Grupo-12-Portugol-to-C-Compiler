
include(FetchContent)

FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0)

FetchContent_MakeAvailable(googletest)
include(GoogleTest)

add_executable(testes_lexico
    lexico/auxiliar_lexer.cpp
    lexico/tokens_validos_test.cpp
    lexico/erros_lexicos_test.cpp
    lexico/cobertura_lexica_test.cpp)

target_link_libraries(testes_lexico
    PRIVATE portugolc_core GTest::gtest_main)
gtest_discover_tests(testes_lexico)

add_executable(testes_sintatico
    sintatico/lacos_test.cpp
    sintatico/condicionais_test.cpp
    sintatico/parser_esqueleto_test.cpp
    sintatico/erros_sintaticos_test.cpp
    sintatico/ast_test.cpp)

target_link_libraries(testes_sintatico
    PRIVATE portugolc_core GTest::gtest_main)

gtest_discover_tests(testes_sintatico)

add_executable(testes_integracao
    integracao/pipeline_test.cpp)

target_link_libraries(testes_integracao
    PRIVATE portugolc_core GTest::gtest_main)

# Os casos sao arquivos .por lidos do disco, entao o teste precisa saber onde
# fica a raiz do repositorio, independente de onde o binario for executado.
target_compile_definitions(testes_integracao
    PRIVATE DIRETORIO_RAIZ="${CMAKE_SOURCE_DIR}")

gtest_discover_tests(testes_integracao)

add_executable(testes_semantico
    semantico/semantico_test.cpp)

target_link_libraries(testes_semantico
    PRIVATE portugolc_core GTest::gtest_main)

gtest_discover_tests(testes_semantico)

add_executable(testes_codegen
    codegen/codegen_test.cpp)

target_link_libraries(testes_codegen
    PRIVATE portugolc_core GTest::gtest_main)

gtest_discover_tests(testes_codegen)

# Issue #67 - Testes ponta a ponta pela interface do compilador
add_executable(testes_e2e
    e2e/cli_test.cpp)

target_link_libraries(testes_e2e
    PRIVATE GTest::gtest_main)

target_compile_definitions(testes_e2e
    PRIVATE PORTUGOLC_BIN="$<TARGET_FILE:portugolc>")

add_dependencies(testes_e2e portugolc)

gtest_discover_tests(testes_e2e)
