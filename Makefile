# Build alternativo ao CMake, util quando o CMake nao esta instalado.
#   make        -> compila build/portugolc
#   make clean  -> apaga build/

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -Iinclude -Isrc -I$(BUILD)
BUILD    := build
SRCS     := $(wildcard src/*.cpp)
BIN      := $(BUILD)/portugolc

# --- Configuracao do Flex (Analisador Lexico) ---
# O Flex le as regras lexicas (.l) e produz um arquivo fonte C++ (.cpp)
FLEX     ?= flex
LEX_SRC  := src/portugolToC.l
LEX_OUT  := $(BUILD)/lex.yy.cpp

.PHONY: all clean

all: $(BIN)

# Regra para chamar o Flex:
# Executa 'flex -o build/lex.yy.cpp src/portugolToC.l' gerando o scanner C++
$(LEX_OUT): $(LEX_SRC) | $(BUILD)
	$(FLEX) -o $@ $<

# Regra de linkagem do executavel final:
# O g++ compila tanto o main.cpp (SRCS) quanto o scanner gerado pelo Flex (LEX_OUT)
$(BIN): $(SRCS) $(LEX_OUT) | $(BUILD)
	$(CXX) $(CXXFLAGS) $(SRCS) $(LEX_OUT) -o $@

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)


