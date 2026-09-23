# Grupo-12-Portugol-to-C-Compiler

Compilador de **Portugol** para **C++**, desenvolvido na disciplina de Compiladores.

## Estrutura do projeto

```
include/portugol/   cabecalhos publicos
src/                implementacao
tests/              testes automatizados
.github/workflows/  integracao continua
```

## Compilando

Com CMake (preferido):

```bash
cmake -S . -B build
cmake --build build
```

Sem CMake, usando o Makefile:

```bash
make
```

Os dois caminhos geram o binario em `build/portugolc`.

## Requisitos

- Compilador C++17 (g++ 9+ ou clang 10+)
- CMake 3.16+ (opcional, ha fallback com Makefile)
