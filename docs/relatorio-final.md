# Relatório final do projeto

Compilador de um dialeto de **Portugol** para **C++17**, desenvolvido pelo Grupo 12 na disciplina de Compiladores 1. Este documento reúne o que o enunciado do trabalho pede no repositório: estrutura do projeto, decisões técnicas, planejamento das sprints e problemas encontrados com as soluções adotadas.

- **Entrada:** um programa `.por` (ver [gramática léxica](gramatica-lexica.md) e [gramática sintática](gramatica-sintatica.md)).
- **Saída:** um programa C++17 completo, que compila sem warnings com `g++ -std=c++17 -Wall -Wextra -Wpedantic`.
- **Qualidade:** 467 testes automatizados (GoogleTest/CTest), executados no GitHub Actions a cada push e pull request, junto com uma verificação de memória com Valgrind.
- **Demonstração:** o roteiro passo a passo está em [demonstracao.md](demonstracao.md).

## 1. Estrutura do projeto

```
.github/workflows/ci.yml   CI: build + testes e verificacao de memoria (Valgrind)
CMakeLists.txt, Makefile   dois caminhos de build (o CMake tambem monta os testes)
docs/                      especificacoes e documentacao do projeto
examples/                  programas Portugol de exemplo (inclui demonstracao.por)
include/portugol/          interfaces publicas: AST, tabela de simbolos, semantico, gerador
src/
  portugolToC.l            analisador lexico (Flex)
  PortugolToC.y            analisador sintatico (Bison) + montagem da AST + tabela
  ast.cpp                  construcao, impressao e liberacao da AST
  tabela.cpp               tabela de simbolos com pilha de escopos
  semantico.cpp            analise semantica sobre a AST
  codegen.cpp              geracao de codigo C++17 a partir da AST anotada
  main.cpp                 linha de comando (portugolc)
tests/
  lexico/                  tokens validos, erros lexicos e cobertura da gramatica
  sintatico/               esqueleto do parser, condicionais, lacos, erros, AST
  integracao/              arquivos .por passando pelo fluxo lexico + sintatico
  tabela/                  escopos, parametros, vetores e redeclaracao
  semantico/               erros e programas validos da analise semantica
  codegen/                 texto gerado e programas compilados/executados
  e2e/                     a CLI de ponta a ponta: .por -> .cpp -> g++ -> execucao
```

## 2. Arquitetura: o caminho de um programa

```
 programa.por
      │
      ▼
 ┌────────────┐  tokens   ┌──────────────┐   AST    ┌─────────────┐  AST anotada  ┌───────────┐
 │  Léxico    │ ────────► │  Sintático   │ ───────► │  Semântico  │ ────────────► │  Gerador  │ ──► programa.cpp
 │  (Flex)    │  yylex()  │  (Bison)     │          │             │  tipoDado,    │  C++17    │
 └────────────┘           │  + tabela de │          └─────────────┘  usado        └───────────┘
                          │  símbolos    │
                          └──────────────┘
```

| Fase | Arquivo | Entrada → saída | O que garante |
|---|---|---|---|
| Léxica | `src/portugolToC.l` | texto → tokens | palavras reservadas, identificadores, literais, operadores e comentários; erros com linha e coluna |
| Sintática | `src/PortugolToC.y` | tokens → AST | estrutura do programa, precedência dos operadores, recuperação de erros; a tabela de símbolos acusa redeclarações |
| Semântica | `src/semantico.cpp` | AST → AST anotada | declarações, tipos, chamadas, `retorne`, `pare`/`continue`; anota o tipo de cada expressão e o uso das variáveis |
| Geração | `src/codegen.cpp` | AST anotada → C++17 | tradução direta, sem repetir verificações |

Cada fase só roda se a anterior terminou sem erros, e o `main.cpp` informa qual delas falhou ("Falha na analise lexica (N erro(s))", "Falha na analise sintatica" ou "Falha na analise semantica (N erro(s))"), saindo com código `1`; nesse caso nenhum `.cpp` é gerado. Erros de uso da linha de comando saem com código `2`.

## 3. Decisões técnicas

### Ferramentas e processo

| Decisão | Por quê |
|---|---|
| Flex + Bison com C++17 | São as ferramentas vistas na disciplina e geram analisadores eficientes a partir de especificações declarativas; C++17 dá `std::string`, contêineres e `std::filesystem` sem dependências externas. |
| C++17 também como linguagem-alvo | Tem tipos equivalentes aos do Portugol (`int`, `double`, `std::string`, `char`, `bool`) e permite verificar a saída compilando-a com `g++ -Werror`. |
| CMake e Makefile | O CMake é o caminho principal (testes, CI); o Makefile atende quem só quer o binário. |
| GoogleTest via `FetchContent` | Não exige instalar o GoogleTest na máquina; o CMake baixa a versão fixada. |
| GitHub Actions com Valgrind | Todo PR roda a suíte completa e uma verificação de vazamentos; nada entra na `devel` sem CI verde. |
| Scrum em 4 sprints, kanban no GitHub Projects, duplas por issue | Recomendação do enunciado; as duplas espalham o conhecimento e garantem que cada parte tenha ao menos duas pessoas que a conhecem. |
| Uma branch por issue (`feat/`, `fix/`, `test/`, `docs/`), PR com revisão para a `devel`, `devel → main` só nas entregas | A `main` guarda apenas versões entregues (P2 e final); a revisão por outra dupla pegou vários bugs antes do merge (ver seção 5). |

### Análise léxica

- **Estados exclusivos (`%x STRING`, `%x CHAR`, `%x COMMENT_BLOCO`)** para cadeias, caracteres e comentários de bloco: o texto é montado caractere a caractere, o que permite tratar sequências de escape e relatar "cadeia não fechada" na posição de abertura.
- **Casamento mais longo** do Flex, com as palavras reservadas antes da regra de identificador: `<<=` vira um único token e `se` vira `KW_SE`, não um identificador. Os casos-limite (`a<<<=b`, `a&&&b`) estão documentados na gramática léxica e testados.
- **Coluna contada por caractere, não por byte**, para que mensagens em linhas com acentos (UTF-8) apontem a coluna certa.
- **Erros léxicos não interrompem a leitura:** o caractere inválido é relatado e ignorado, para que os erros seguintes também apareçam; no fim, o scanner informa quantos erros houve e o programa é recusado.

### Análise sintática

- **Precedência declarada no Bison** (`%left`/`%right`, da atribuição até os unários), em vez de uma gramática estratificada por níveis: a gramática fica curta e a ordem de precedência fica visível num só lugar.
- **Bloco obrigatório em `se`/`senao`/`enquanto`/`para`:** elimina o problema do *dangling else* sem regras extras, e `senao se` é uma alternativa própria da regra condicional.
- **Recuperação de erros com `error PONTO_VIRGULA`:** depois de um erro, o parser descarta tokens até o próximo `;` e continua, relatando vários erros numa só execução.
- **`%locations` e `parse.error detailed`:** cada nó da AST guarda a linha real do código, e as mensagens dizem qual token era inesperado.
- **`%initial-action`** zera o estado global (contadores de erro, tabela de símbolos, AST) no início de cada análise; sem isso, uma análise que falhava contaminava as seguintes (ver seção 5).

### AST e tabela de símbolos

- **Um único tipo de nó (`NoAST`)** com `esq`, `dir`, `extra` e uma lista `filhos`, mais o campo `tipo` (`TipoNo`). É simples de construir nas ações do Bison e de percorrer; as fases seguintes acrescentam anotações no próprio nó (`linha`, `tipoDado`, `usado`).
- **Tabela de símbolos com pilha de escopos** (global, função, bloco e `para`), preenchida durante o parse. Parâmetros e corpo de uma função dividem o mesmo escopo, assim como a variável de um `para` e o corpo do laço, seguindo as regras do C/C++. Os símbolos ficam num `std::deque`, que não invalida os ponteiros guardados nos escopos; os símbolos de escopos fechados continuam disponíveis para a impressão da tabela. Detalhes em [tabela-simbolos.md](tabela-simbolos.md).

### Análise semântica

- **Passada separada sobre a AST**, em vez de verificar tudo dentro das ações do Bison: a gramática não fica poluída, e a análise enxerga o programa inteiro.
- **Pré-passada que registra todas as funções** antes de analisar os corpos, para que uma função possa chamar outra definida mais abaixo (e a si mesma).
- **Tipo `TIPO_ERRO`** para expressões já erradas: toda verificação que envolve uma expressão com esse tipo é pulada, então um erro não gera uma cascata de mensagens falsas na mesma expressão.
- **Conversões implícitas só quando não perdem informação:** `inteiro → real` e `caracter → cadeia`.
- **Retorno em todos os caminhos:** uma função não `vazio` que pode terminar sem `retorne` é recusada (`sempreRetorna` considera `se/senao` completos e laços infinitos sem `pare`).
- **Todos os erros são relatados** de uma vez, cada um com a linha de origem, no formato `Erro semântico na linha N: ...`.

### Geração de código

- **Tradução direta da AST anotada:** o gerador não repete verificações; ele confia na análise semântica e usa as anotações dela.
- **Parênteses explícitos em toda operação aninhada:** a precedência do Portugol fica garantida no C++ e o `g++` não emite `-Wparentheses`.
- **Literais de cadeia em operações viram `std::string("...")`:** em C++, `"a" + "b"` somaria ponteiros.
- **Literal real sempre com ponto** (`2.0`, nunca `2`): senão `4.0 / 2` viraria divisão inteira.
- **Prefixo `p_` em todo identificador:** evita colisões com palavras reservadas, nomes da biblioteca padrão e macros (`int`, `div`, `time`, `EOF`), mantendo a tradução injetiva (`int` e `int_` viram `p_int` e `p_int_`).
- **Renomeação de variáveis que escondem outras (`p2_x`):** em C++ o nome novo já vale dentro do próprio inicializador; sem isso, `inteiro x = x + 1;` leria a variável nova, ainda sem valor.
- **`caracter` entregue a `cadeia` vira `std::string(1, c)`**, porque `std::string s = 'a';` não compila.
- **Variáveis sem inicializador recebem o valor padrão do tipo** (`0`, `0.0`, `""`, `'\0'`, `false`), e as nunca lidas ganham `[[maybe_unused]]`, para o código compilar limpo com `-Wall -Wextra`.
- **Protótipos de todas as funções** antes das definições, e `principal()` vira `int main()`; um programa sem funções vira o corpo de `main`.

## 4. Planejamento e execução das sprints

O backlog foi organizado em issues com rótulo de sprint, dupla responsável e critérios de aceite, acompanhadas no kanban do GitHub Projects ("Compilador Portugol -> C++").

| Sprint | Período | Objetivo | Marco |
|---|---|---|---|
| 1 | a partir de 22/09 | estrutura do projeto, build e analisador léxico | — |
| 2 | até 10/10 | integração Flex/Bison, gramática sintática e esqueleto do parser | **P1** |
| 3 | 10/10 a 10/11 | parser completo, AST e tabela de símbolos | **P2** (release `devel → main`, PR #63) |
| 4 | 12/11 a 29/11 | escopos, análise semântica, geração de código, testes E2E e documentação | **Entrega final** |

### Sprint 1: léxico

| Issue | Tarefa | Dupla |
|---|---|---|
| #1 | Estrutura do projeto e build system | Vinícius, Daniel |
| #2 | Especificação da gramática léxica | Vinícius, Arthur |
| #3, #4, #5, #12 | Flex: palavras reservadas e identificadores, literais, operadores, comentários e espaços | Vinícius com Arthur, Cadu e Daniel |
| #6, #7 | Testes de tokens válidos e de erros léxicos | Arthur, Mariana |
| #8 | README e docstrings do lexer | Cadu, Mariana |
| #9 | Pipeline do Flex no CMake/Makefile | Vinícius, Daniel |
| #10 | Padronização dos erros léxicos | Daniel, Mariana |
| #20 | CLI do modo scanner (`--tokens`) | Vinícius, Mariana |

### Sprint 2: integração e esqueleto do parser (P1)

| Issue | Tarefa | Dupla |
|---|---|---|
| #11 | Documentar a arquitetura do lexer | Cadu, Mariana |
| #13 | Interface Flex (`yylex`) ↔ Bison (`yyparse`) | Cadu, Daniel |
| #14 | Gramática livre de contexto (EBNF) | Cadu, Arthur |
| #15 | Cobertura completa da gramática léxica | Daniel, Arthur |
| #16, #17 | Testes de integração lexer + parser e do esqueleto do parser | Vinícius e Mariana; Arthur e Mariana |
| #18 | Esqueleto do analisador sintático | Cadu, Arthur |
| #19 | Regras de `se`/`senao` | Vinícius, Daniel |
| #21 | Recuperação de erros no scanner | Vinícius, Mariana |
| #23 | Slides e formulário do P1 | Daniel, Mariana |

### Sprint 3: parser completo, AST e tabela (P2)

| Issue | Tarefa | Dupla |
|---|---|---|
| #30 | Laços `enquanto` e `para` | Vinícius, Daniel |
| #31, #32 | Funções e chamadas; `leia` e `escreva` | Cadu, Arthur |
| #33 | Mensagens e recuperação de erros sintáticos | Daniel, Mariana |
| #34 | Suíte de testes sintáticos no CI | Vinícius, Mariana |
| #35 | Slides, documentação e formulário do P2 | Cadu, Mariana |
| #36 | Estrutura e impressão da AST | Daniel, Arthur |
| #37 | Tabela de símbolos básica | Vinícius, Cadu |

### Sprint 4: semântica, geração e entrega final

| Issue | Tarefa | Dupla |
|---|---|---|
| #64 | Tabela de símbolos com escopos, parâmetros e vetores | Vinícius, Cadu |
| #65 | Analisador semântico | Daniel, Arthur |
| #66 | Gerador de código C++17 | Vinícius, Daniel |
| #67 | Testes semânticos e E2E no CI (com Valgrind) | Arthur, Mariana |
| #68 | Documentação final, demonstração e release na `main` | Cadu, Mariana (com apoio da equipe) |

### Execução e ajustes no planejamento

- **Testes e documentação do léxico atrasaram:** #6, #7, #8, #10 e #15 foram planejados para as sprints 1 e 2, mas fecharam entre outubro e novembro. A implementação do léxico ficou pronta cedo (PRs #27 a #29, até 28/09), e esses itens foram concluídos junto com o parser, quando a interface entre as fases já estava estável.
- **Bugs viraram issues próprias:** #41 (CRLF), #50 (coluna UTF-8) e #60 (AST sem funções) foram abertos fora do planejamento, assim que apareceram, e resolvidos dentro da sprint em andamento.
- **Ordem da Sprint 4:** a #64 (escopos) tinha prazo em 15/11, mas terminou em 27/11, depois do semântico e do gerador. Para não bloquear, o semântico ganhou escopos próprios e o gerador foi escrito já preparado para o sombreamento; quando a #64 entrou, bastou acrescentar testes (ver seção 5).
- **Integração por PRs empilhados:** o gerador (#70) dependia do semântico (#69) e os testes E2E (#71) dependiam do gerador. Cada PR foi aberto sobre o anterior e teve a base trocada para a `devel` quando o anterior entrou.

## 5. Problemas encontrados e soluções

| Problema | Causa | Solução | Onde |
|---|---|---|---|
| Arquivos com final de linha do Windows geravam um erro léxico por linha | o padrão de espaço era `[ \t]+`, sem `\r` | `\r` passou a ser espaço, como já dizia a gramática léxica | #41, PR #43 |
| Coluna errada em erros depois de letras acentuadas | o Flex avança `yyleng`, que conta bytes; `á` em UTF-8 tem 2 | a coluna passou a contar caracteres (bytes de continuação UTF-8 não avançam) | #50, PR #59 |
| Funções, chamadas, `retorne`, `leia` e `escreva` não apareciam na AST | as regras do parser aceitavam essas construções, mas não criavam nós | novos tipos de nó e construtores (`criarNoFuncao`, `criarNoChamada`...) | #60, PR #61 |
| Uma análise com erro fazia todas as seguintes falharem (nos testes, várias análises rodam no mesmo processo) | `yynerrs` e o estado global do parser não voltavam a zero entre chamadas de `yyparse()` | `%initial-action` reinicia contadores, tabela e AST | PR #54 |
| A linha guardada nos nós da AST às vezes era a do token seguinte | o nó usava `yylineno`, que já tinha avançado pelo *lookahead* | uso das localizações do Bison (`@$`, `%locations`) | PR #69 |
| Programas válidos eram recusados por "redeclaração" (`inteiro r` em duas funções, dois `para (inteiro i ...)`) | a tabela de símbolos tinha um escopo só | pilha de escopos com as mesmas regras do semântico | #64, PR #72 |
| Redeclaração aparecia como "Falha na analise sintatica" | a tabela acusa a redeclaração durante o parse, e o `yyparse()` falha | o parser expõe a contagem de redeclarações e o driver relata falha semântica | PR #72 |
| `"a" + "b"`, `4.0 / 2` e `a + b * c` com significado diferente no C++ | ponteiros em vez de cadeias, literal real virando inteiro, precedência | `std::string("...")`, literal real com ponto e parênteses explícitos | #66, PR #70 |
| Na revisão do gerador: `cadeia s = 'a';` não compilava, `inteiro x = x + 1;` imprimia lixo, nomes como `div`/`EOF` colidiam com o C++, e `-o x.por x.por` apagava o programa | diferenças entre as regras do Portugol e as do C++ | `std::string(1, c)`, renomeação `p2_x`, prefixo `p_` e recusa de saída igual à entrada | revisão do PR #70 |
| Um programa com caractere inválido (`@`) gerava C++ e o compilador saía com código 0 | o scanner relatava o erro e ignorava o caractere, mas ninguém contava os erros léxicos | o scanner conta os erros e o driver recusa o programa ("Falha na analise lexica") | issue #68 |
| Um arquivo novo em `src/` não era compilado | o `CMakeLists.txt` usa `file(GLOB ...)`, avaliado só ao configurar | reconfigurar com `cmake -S . -B build` ao criar arquivos | processo |

## 6. Testes

| Suíte | Testes | O que cobre |
|---|---|---|
| `tests/lexico` | 250 | todos os tokens, escapes, casos-limite do casamento mais longo e erros léxicos com linha e coluna |
| `tests/sintatico` | 88 | esqueleto, condicionais, laços, mensagens e recuperação de erros, formato da AST |
| `tests/integracao` | 17 | arquivos `.por` (incluindo os de `examples/`) passando pelo fluxo completo |
| `tests/tabela` | 26 | escopos, sombreamento, parâmetros, assinatura de funções, vetores e redeclaração |
| `tests/semantico` | 48 | cada erro semântico e programas válidos que não podem ser recusados |
| `tests/codegen` | 22 | texto gerado e programas compilados com `g++ -Werror` e executados |
| `tests/e2e` | 16 | a CLI de ponta a ponta, incluindo a demonstração, entrada padrão, mensagens de falha de cada fase e códigos de saída |
| **Total** | **467** | |

O CI do GitHub Actions compila o projeto, roda a suíte inteira e, num segundo job, executa o compilador sob o Valgrind nos caminhos de sucesso e de erro semântico, falhando se houver vazamento definitivo ou indireto.

## 7. Limitações conhecidas

- Vetores são unidimensionais, têm tamanho literal, não podem ser inicializados na declaração e não podem ser parâmetros nem retorno de funções (`funcao f(inteiro v[])` é erro de sintaxe).
- Não há `faca ... enquanto`, `escolha`/`caso` nem constantes.
- `leia` lê com `std::cin >>`: uma `cadeia` é lida até o primeiro espaço.
