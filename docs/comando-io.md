# Comandos de Entrada e Saída

Regras adicionadas em `src/PortugolToC.y` para `leia` e `escreva`. As
regras existentes (`funcao`, `comando_retorno`, `chamada_funcao`,
`declaracao_variavel`, `comando_atribuicao`, expressões) **não foram
alteradas**.

## `leia(<variavel>);`

Regra:

```bison
comando_leia:
      KW_LEIA ABRE_PAREN acesso FECHA_PAREN PONTO_VIRGULA
    ;
```

O destino é um `acesso`: uma variável ou uma posição de vetor. `leia();` e
`leia(x + 1);` são recusados.

## `escreva(<expressao>, ...);`

Regra:

```bison
comando_escreva:
      KW_ESCREVA ABRE_PAREN lista_expressoes_escrita FECHA_PAREN PONTO_VIRGULA
    ;
```

Aceita uma ou mais expressões separadas por vírgula. `escreva();` é recusado.

## Representação na AST

| Comando | Nó | Impressão |
|---|---|---|
| `leia(x);` | `NO_LEIA`, com o destino da leitura | `(leia x)` |
| `leia(v[2]);` | `NO_LEIA`, com o acesso ao vetor | `(leia v[2])` |
| `escreva("x = ", x + 1);` | `NO_ESCREVA`, com uma expressão por argumento | `(escreva "x = " (+ x 1))` |
