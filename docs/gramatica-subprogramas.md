# Subprogramas e Modularização

Regras **adicionadas** em `src/PortugolToC.y`. As declarações de `%token`,
`%left`/`%right`, `%start`, `%union`, `%destructor` e as regras de
declaração de variáveis, atribuição e expressões preexistentes
**não foram alteradas**.

## Funções

`funcao <tipo> <nome>(<parametros>) { <comandos> }`

| Portugol  | Descrição      |
|-----------|----------------|
| inteiro   | int            |
| real      | float          |
| cadeia    | string         |
| caracter  | char           |
| logico    | bool           |
| vazio     | função sem retorno |

Tanto o tipo de retorno quanto o tipo de cada parâmetro são
reaproveitados da regra `tipo` já existente; `vazio` só é aceito
em posição de retorno (`tipo_retorno: tipo | KW_VAZIO`).

## Parâmetros

`lista_parametros` aceita:

- zero parâmetros: `principal()`
- um parâmetro: `dobro(inteiro x)`
- múltiplos: `soma(inteiro a, inteiro b)`

Cada `parametro` é `tipo IDENTIFICADOR`. A lista vazia e a não vazia são
produções separadas (`lista_parametros: %empty | parametros`), então a vírgula
só aparece entre dois parâmetros: `f(, inteiro a)` e `f(inteiro a,)` são
recusados. `lista_argumentos` segue o mesmo arranjo.

## Retorno

`comando_retorno` cobre:

- `retorne <expressao>;`
- `retorne;` (uso esperado em funções `vazio`)

A restrição de que `retorne;` só aparece em funções do tipo
`vazio` fica a cargo da tabela de símbolos (#37), junto com a
verificação de tipo da expressão retornada.

## Chamada de função como comando

`comando_chamada: chamada_funcao PONTO_VIRGULA` permite chamar uma função
descartando o valor, como em `mostrar(total);`. Não há conflito com
`comando_atribuicao`: depois do `IDENTIFICADOR`, o `ABRE_PAREN` leva à
chamada e um operador de atribuição ou `ABRE_COLCHETE` leva ao `acesso`.

## Chamada de função como termo

`chamada_funcao: IDENTIFICADOR ABRE_PAREN lista_argumentos FECHA_PAREN`

foi adicionada como alternativa de `expressao`. Com isso, chamadas
podem aparecer em qualquer posição onde uma `expressao` é válida,
por exemplo:

- `x = soma(a, b);`
- `x = soma(a, b) + 1;`
- `retorne dobro(f(x));`

A ambiguidade entre `acesso: IDENTIFICADOR` e
`chamada_funcao: IDENTIFICADOR '(' ...` é resolvida naturalmente
pelo LALR: `ABRE_PAREN` não pertence a `FOLLOW(acesso)` nesse
contexto, então o parser dá shift para `chamada_funcao`.

A precedência e associatividade continuam sendo as das diretivas
`%left`/`%right` já presentes (ex.: `%left MAIS MENOS` antes de
`%left VEZES DIVIDIDO RESTO` mantém `a + b * c` como `a + (b*c)`).

## Obrigatoriedade de `principal()`

Uma flag global `tem_principal` é marcada na ação de `funcao` quando
o identificador é `principal`. Na ação de `programa` (nas duas
alternativas: com e sem `KW_PROGRAMA { ... }`), se a flag
permanecer `false`, é emitida mensagem via `yyerror` e a análise
é abortada via `YYABORT`.

## Rastreabilidade com os critérios de aceite

| Critério                                           | Onde                                                         |
|----------------------------------------------------|--------------------------------------------------------------|
| Função sem parâmetros                              | `lista_parametros: %empty`                                   |
| Função com múltiplos parâmetros                    | `lista_parametros: parametro | lista_parametros VIRGULA parametro` |
| `retorne <expr>;` e `retorne;`                     | `comando_retorno`                                            |
| Chamada de função como termo                       | `expressao: chamada_funcao`                                  |
| `funcao vazio principal()`                         | `tipo_retorno: KW_VAZIO` + identificador `principal`         |
| Obrigatoriedade de `principal()`                   | Flag `tem_principal` + checagem em `programa`                |
| Chamada de função como comando                     | `item: comando_chamada`                                      |
| Representação na AST                               | Nós `NO_FUNCAO`, `NO_PARAMETRO`, `NO_CHAMADA` e `NO_RETORNE` |

## Representação na AST

| Construção | Nó | Impressão |
|---|---|---|
| Função | `NO_FUNCAO`: tipo de retorno, nome, parâmetros e corpo | `(funcao inteiro soma (inteiro a, inteiro b) (bloco ...))` |
| Parâmetro | `NO_PARAMETRO`: tipo e nome | `inteiro a` |
| Chamada | `NO_CHAMADA`: nome e argumentos | `(chamada soma a 2)` |
| Retorno | `NO_RETORNE`: expressão opcional | `(retorne (+ a b))` / `(retorne)` |

## Limitações

- A validação de tipos de argumentos, aridade e do tipo de retorno fica
  para a análise semântica.
- A restrição de que `retorne;` só aparece em funções `vazio` também é
  verificação semântica.
- Os parâmetros não entram na tabela de símbolos: ela ainda não tem escopos,
  e registrar parâmetros num escopo único faria duas funções com parâmetros
  de mesmo nome acusarem redeclaração.
- A gramática aceita uma função declarada dentro do corpo de outra, porque o
  corpo é um `bloco` e o `bloco` aceita qualquer `item`. C++ não permite
  funções aninhadas, então isso precisa ser recusado antes da geração de código.
