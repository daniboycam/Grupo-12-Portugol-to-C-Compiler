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