cmake_minimum_required(VERSION 3.16)
project(portugolc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -Wpedantic)

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

# Bison (analisador sintatico)
set(PARSER_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/PortugolToC.y)
set(PARSER_OUT ${CMAKE_CURRENT_BINARY_DIR}/PortugolToC.tab.cpp)
set(PARSER_HDR ${CMAKE_CURRENT_BINARY_DIR}/PortugolToC.tab.hpp)
BISON_TARGET(Parser ${PARSER_SRC} ${PARSER_OUT} DEFINES_FILE ${PARSER_HDR})

# Flex (analisador lexico)
set(SCANNER_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/portugolToC.l)
set(SCANNER_OUT ${CMAKE_CURRENT_BINARY_DIR}/lex.yy.cpp)
FLEX_TARGET(Scanner ${SCANNER_SRC} ${SCANNER_OUT})

# Dependencia: Bison gera o cabecalho antes do Flex compilar
ADD_FLEX_BISON_DEPENDENCY(Scanner Parser)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

file(GLOB PORTUGOLC_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
list(REMOVE_ITEM PORTUGOLC_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)  # ajuste ao nome real

add_library(portugolc_core STATIC ${PORTUGOLC_SRCS} ${FLEX_Scanner_OUTPUTS})
add_executable(portugolc ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
target_link_libraries(portugolc PRIVATE portugolc_core)

enable_testing()
add_subdirectory(tests)


