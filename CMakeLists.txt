cmake_minimum_required(VERSION 3.16)
project(portugolc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -Wpedantic)

# Localiza o utilitário do Flex instalado no sistema operacional
find_package(FLEX REQUIRED)

# Define onde está o arquivo de regras (.l) e onde o arquivo C++ gerado (.cpp) será salvo
set(SCANNER_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/portugolToC.l)
set(SCANNER_OUT ${CMAKE_CURRENT_BINARY_DIR}/lex.yy.cpp)

# Cria a tarefa de compilação: o CMake chamará o Flex para gerar o scanner C++
FLEX_TARGET(Scanner ${SCANNER_SRC} ${SCANNER_OUT})

# Diretórios de cabeçalho: permite encontrar os headers do projeto e os arquivos gerados no build
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Compila os fontes C++ (.cpp) juntamente com a saída gerada pelo Flex (${FLEX_Scanner_OUTPUTS})
file(GLOB PORTUGOLC_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_executable(portugolc ${PORTUGOLC_SRCS} ${FLEX_Scanner_OUTPUTS})


