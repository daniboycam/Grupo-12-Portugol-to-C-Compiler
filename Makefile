CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -Iinclude -Isrc -I$(BUILD)
BUILD    := build
SRCS     := $(wildcard src/*.cpp)
BIN      := $(BUILD)/portugolc

# Bison
BISON      ?= bison
BISON_SRC  := src/PortugolToC.y
BISON_OUT  := $(BUILD)/PortugolToC.tab.cpp
BISON_HDR  := $(BUILD)/PortugolToC.tab.hpp

# Flex
FLEX       ?= flex
LEX_SRC    := src/portugolToC.l
LEX_OUT    := $(BUILD)/lex.yy.cpp

.PHONY: all clean test

all: $(BIN)

$(BISON_OUT) $(BISON_HDR): $(BISON_SRC) | $(BUILD)
	$(BISON) -d -o $(BISON_OUT) $<

$(LEX_OUT): $(LEX_SRC) $(BISON_HDR) | $(BUILD)
	$(FLEX) -o $@ $<

$(BIN): $(SRCS) $(BISON_OUT) $(LEX_OUT) | $(BUILD)
	$(CXX) $(CXXFLAGS) $(SRCS) $(BISON_OUT) $(LEX_OUT) -o $@

$(BUILD):
	mkdir -p $(BUILD)

# Os testes usam GoogleTest/CTest, configurados pelo CMake; este alvo so
# delega, numa pasta propria para nao misturar com o build do Makefile.
TEST_BUILD := build-testes

test:
	cmake -S . -B $(TEST_BUILD)
	cmake --build $(TEST_BUILD) --parallel
	ctest --test-dir $(TEST_BUILD) --output-on-failure

clean:
	rm -rf $(BUILD) $(TEST_BUILD)
