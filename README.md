# Grupo-12-Portugol-to-C-Compiler

Compilador de **Portugol** para **C++**, desenvolvido na disciplina de Compiladores.

## Integrantes

| [<img src="https://github.com/ArthurFeitosa05.png" width="80px;" alt="Arthur Feitosa"/><br /><sub><b>Arthur Feitosa</b></sub>](https://github.com/ArthurFeitosa05) | [<img src="https://github.com/kdukoelho.png" width="80px;" alt="Cadu Coelho"/><br /><sub><b>Cadu Coelho</b></sub>](https://github.com/kdukoelho) | [<img src="https://github.com/daniboycam.png" width="80px;" alt="Daniel Batista"/><br /><sub><b>Daniel Batista</b></sub>](https://github.com/daniboycam) | [<img src="https://github.com/marispmorais.png" width="80px;" alt="Mariana Morais"/><br /><sub><b>Mariana Morais</b></sub>](https://github.com/marispmorais) | [<img src="https://github.com/viniiribeiro.png" width="80px;" alt="Vinícius Ribeiro"/><br /><sub><b>Vinícius Ribeiro</b></sub>](https://github.com/viniiribeiro) |
| :---: | :---: | :---: | :---: | :---: |

## Estrutura do projeto

```
docs/               documentacao e especificacoes das gramaticas (lexica e sintatica)
examples/           programas de exemplo em Portugol (.por)
include/portugol/   cabecalhos publicos (tokens, AST e tabela de simbolos)
src/                implementacao (C++, Flex e Bison)
tests/              testes automatizados (lexico, sintatico e integracao)
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

## Usando

### Analise sintatica, analise semantica, Tabela de Simbolos e AST (modo padrao)

Sem `--tokens`, o compilador executa a analise lexica e sintatica completa, exibe a Tabela de Simbolos e imprime a Arvore Sintatica Abstrata (AST) em formato S-expression:

```bash
./build/portugolc examples/condicional.por
```

```
===== Tabela de Simbolos =====
nome	tipo	linha
nota	inteiro	3
conceito	cadeia	4
==============================
Analise sintatica concluida com sucesso.
Analise semantica concluida com sucesso.
(programa
  (decl inteiro (nota = 75))
  (decl cadeia (conceito = ""))
  (se (>= nota 90)
    (bloco
      (= conceito "A")
    )
    ...
  )
)
```

### Erros semanticos

Depois do parser, a analise semantica percorre a AST e relata todos os erros encontrados, com a linha de origem, terminando com codigo de saida `1`:

```
Erro semântico na linha 2: nao e possivel inicializar 'x' (inteiro) com um valor do tipo cadeia
Erro semântico na linha 3: condicao do 'se' deve ser do tipo logico (recebeu inteiro)
Erro semântico na linha 4: 'pare' so pode ser usado dentro de um laco (enquanto/para)
Erro semântico na linha 6: variavel 'y' nao declarada
Falha na analise semantica (4 erro(s)).
```

Sao verificados: uso de variaveis e funcoes nao declaradas, redeclaracao no mesmo escopo, compatibilidade de tipos em inicializacoes, atribuicoes e expressoes (sao aceitas as conversoes `inteiro -> real` e `caracter -> cadeia`), chamadas de funcao (existencia, aridade e tipo dos argumentos), `retorne` (tipo compativel, `retorne;` so em funcoes `vazio` e retorno em todos os caminhos), `pare`/`continue` apenas dentro de lacos, indices de vetor inteiros e funcoes declaradas apenas no nivel do programa. As funcoes podem ser chamadas antes de sua definicao (inclusive recursivamente); variaveis precisam ser declaradas antes do uso.

### Gerando codigo C++17 (`-o`)

Com `-o <arquivo.cpp>`, um programa valido e traduzido para C++17 (use `-o -` para escrever na saida padrao):

```bash
./build/portugolc -o programa.cpp examples/lacos.por
g++ -std=c++17 -Wall -Wextra -Wpedantic programa.cpp -o programa
./programa
```

Traducao: `inteiro -> int`, `real -> double`, `cadeia -> std::string`, `caracter -> char`, `logico -> bool`, `vazio -> void`; `funcao principal()` vira `int main()` (um programa sem funcoes vira o corpo de `main`); `leia` -> `std::cin >>`, `escreva` -> `std::cout <<` (valores logicos saem como `verdadeiro`/`falso`); `se/senao`, `enquanto`, `para`, `pare`, `continue` e `retorne` viram `if/else`, `while`, `for`, `break`, `continue` e `return`. Variaveis sem inicializador recebem o valor padrao do tipo e identificadores que sao palavras reservadas do C++ ganham o sufixo `_`. O codigo gerado compila sem warnings com `-Wall -Wextra -Wpedantic`.

### Modo scanner (`--tokens`)

Com `-t` ou `--tokens`, o programa percorre a entrada apenas com o analisador lexico e lista os tokens reconhecidos:

```bash
./build/portugolc --tokens examples/exemplo.por
```

```
LINHA   TOKEN                 LEXEMA
2       KW_PROGRAMA           programa
2       ABRE_CHAVE            {
3       KW_FUNCAO             funcao
...
```

Sem arquivo, le da entrada padrao. `-h` ou `--ajuda` mostra as opcoes.

## Executando os testes

Apos compilar com CMake, execute a suite de testes automatizados (GoogleTest / CTest):

```bash
ctest --test-dir build --output-on-failure
```

Ou via Makefile:

```bash
make test
```

## Documentacao

- [Gramatica Lexica](docs/gramatica-lexica.md)
- [Gramatica Sintatica](docs/gramatica-sintatica.md)
- [Subprogramas e Modularizacao](docs/gramatica-subprogramas.md)
- [Comandos de Entrada e Saida](docs/comando-io.md)

## Requisitos

- Compilador C++17 (g++ 9+ ou clang 10+)
- CMake 3.16+ ou GNU Make
- Flex 2.6+
- Bison 3.5+
