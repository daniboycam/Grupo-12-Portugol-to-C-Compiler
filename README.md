# Grupo-12-Portugol-to-C-Compiler

Compilador de **Portugol** para **C++**, desenvolvido na disciplina de Compiladores.

## Integrantes

| [<img src="https://github.com/ArthurFeitosa05.png" width="80px;" alt="Arthur Feitosa"/><br /><sub><b>Arthur Feitosa</b></sub>](https://github.com/ArthurFeitosa05) | [<img src="https://github.com/kdukoelho.png" width="80px;" alt="Cadu Coelho"/><br /><sub><b>Cadu Coelho</b></sub>](https://github.com/kdukoelho) | [<img src="https://github.com/daniboycam.png" width="80px;" alt="Daniel Batista"/><br /><sub><b>Daniel Batista</b></sub>](https://github.com/daniboycam) | [<img src="https://github.com/marispmorais.png" width="80px;" alt="Mariana Morais"/><br /><sub><b>Mariana Morais</b></sub>](https://github.com/marispmorais) | [<img src="https://github.com/viniiribeiro.png" width="80px;" alt="Vinícius Ribeiro"/><br /><sub><b>Vinícius Ribeiro</b></sub>](https://github.com/viniiribeiro) |
| :---: | :---: | :---: | :---: | :---: |

## Estrutura do projeto

```
docs/               documentacao e especificacoes
include/portugol/   cabecalhos publicos
src/                implementacao (C++, Flex e Bison)
tests/              testes automatizados
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
- CMake 3.16+ ou GNU Make
- Flex 2.6+
- Bison 3.5+
