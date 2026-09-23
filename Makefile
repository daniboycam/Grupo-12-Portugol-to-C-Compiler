# Build alternativo ao CMake, util quando o CMake nao esta instalado.
#   make        -> compila build/portugolc
#   make clean  -> apaga build/

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -Iinclude
BUILD    := build
SRCS     := $(wildcard src/*.cpp)
BIN      := $(BUILD)/portugolc

.PHONY: all clean

all: $(BIN)

$(BIN): $(SRCS) | $(BUILD)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $@

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)
