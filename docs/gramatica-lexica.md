# Gramática léxica do Portugol (dialeto do Grupo 12)

Este documento define **tudo o que o lexer deve reconhecer**: palavras reservadas,
tipos, identificadores, literais, operadores, delimitadores e comentários.

O dialeto é próprio do grupo: inspirado no Portugol Studio, mas reduzido ao que
cabe no semestre. A grafia é sempre **sem acentos** nas palavras reservadas
(`funcao`, `logico`, `senao`), para não depender de codificação do arquivo.

Escopo deste documento: apenas o nível **léxico** (como o texto vira tokens).

## 1. Convenções da notação

EBNF, com:

| Notação | Significado |
|---|---|
| `"texto"` | terminal literal |
| `a \| b` | alternativa |
| `[ a ]` | opcional (0 ou 1) |
| `{ a }` | repetição (0 ou mais) |
| `( a )` | agrupamento |
| `a - b` | tudo que casa com `a` e não casa com `b` |

Arquivos-fonte são lidos como **UTF-8**.

## 2. Estrutura geral da entrada

```ebnf
arquivo  = { token | espaco | comentario } ;
token    = palavra_reservada | identificador | literal | operador | delimitador ;
```

Espaços e comentários **não geram token**: são separadores e podem aparecer
entre quaisquer dois tokens.

```ebnf
espaco   = " " | "\t" | "\r" | "\n" ;
```

A quebra de linha não é significativa (não termina comando); serve apenas para
contar linhas nas mensagens de erro. O fim do arquivo produz o token `FIM_ARQUIVO`.

### Regra do casamento mais longo

Quando duas regras casam no mesmo ponto, vence a de **maior comprimento**
(`>=` é um token, não `>` seguido de `=`; `senaose` seria um identificador, não
`senao` + `se`). Empatando o comprimento, palavra reservada vence identificador.

## 3. Palavras reservadas

São **reservadas**: nunca podem ser usadas como identificador. Todas em
minúsculas — a linguagem é *case sensitive*, então `Se` é um identificador válido
e distinto de `se`.

| Categoria | Palavras |
|---|---|
| Estrutura | `programa`, `funcao`, `retorne` |
| Tipos | `inteiro`, `real`, `cadeia`, `caracter`, `logico`, `vazio` |
| Condicional | `se`, `senao` |
| Repetição | `enquanto`, `para`, `pare`, `continue` |
| Literais lógicos | `verdadeiro`, `falso` |
| Entrada/saída | `leia`, `escreva` |

```ebnf
palavra_reservada =
    "programa" | "funcao"  | "retorne"
  | "inteiro"  | "real"    | "cadeia"  | "caracter" | "logico" | "vazio"
  | "se"       | "senao"
  | "enquanto" | "para"    | "pare"    | "continue"
  | "verdadeiro" | "falso"
  | "leia"     | "escreva" ;
```

> `leia` e `escreva` são palavras reservadas neste dialeto, e não funções de
> biblioteca: o parser trata os dois como comandos próprios, sem precisar de uma
> tabela de símbolos pré-carregada.

### Tipos

| Tipo | Domínio | Equivalente em C++ |
|---|---|---|
| `inteiro` | números inteiros com sinal | `int` |
| `real` | ponto flutuante | `double` |
| `cadeia` | sequência de caracteres | `std::string` |
| `caracter` | um caractere | `char` |
| `logico` | `verdadeiro` / `falso` | `bool` |
| `vazio` | ausência de valor (retorno de função) | `void` |

## 4. Identificadores

```ebnf
identificador = ( letra | "_" ) { letra | digito | "_" } ;
letra         = "a".."z" | "A".."Z" ;
digito        = "0".."9" ;
```

- Não podem começar por dígito: `2x` é erro léxico.
- Sem acentos e sem `ç` (mantém o lexer em ASCII fora das cadeias).
- Sem limite de comprimento.
- Uma sequência que case com `identificador` é classificada como
  `PALAVRA_RESERVADA` se estiver na tabela da seção 3; senão, `IDENTIFICADOR`.

Válidos: `x`, `soma_total`, `_temp`, `valor2`, `Se`.
Inválidos: `2x` (começa com dígito), `preço` (acento), `meu-valor` (hífen não é letra).

## 5. Literais

### 5.1 Inteiro

```ebnf
literal_inteiro = digito { digito } ;
```

Apenas base decimal. O sinal negativo **não** faz parte do literal: `-5` são dois
tokens (`-` e `5`), resolvidos como operador unário pelo parser.

Exemplos: `0`, `7`, `1024`, `007` (válido, vale 7).

### 5.2 Real

```ebnf
literal_real = digito { digito } "." digito { digito } [ expoente ]
             | digito { digito } expoente ;
expoente     = ( "e" | "E" ) [ "+" | "-" ] digito { digito } ;
```

Exige pelo menos um dígito de cada lado do ponto.

Válidos: `3.14`, `0.5`, `1.0e10`, `2E-3`.
Inválidos: `3.` (sem parte decimal), `.5` (sem parte inteira), `1e` (expoente vazio).

### 5.3 Cadeia

```ebnf
literal_cadeia = '"' { caractere_cadeia | escape } '"' ;
caractere_cadeia = qualquer_caractere - ( '"' | "\" | "\n" ) ;
```

- Delimitada por aspas duplas.
- **Não pode** atravessar linha; quebra antes do fechamento é erro léxico.
- Pode conter acentos e qualquer caractere UTF-8.

Exemplos: `"ola"`, `"linha 1\nlinha 2"`, `""` (cadeia vazia).

### 5.4 Caracter

```ebnf
literal_caracter = "'" ( caractere_simples | escape ) "'" ;
caractere_simples = qualquer_caractere - ( "'" | "\" | "\n" ) ;
```

Exatamente um caractere entre aspas simples. `''` (vazio) e `'ab'` são erros.

### 5.5 Sequências de escape

Válidas dentro de cadeia e de caracter:

| Escape | Significado |
|---|---|
| `\n` | nova linha |
| `\t` | tabulação |
| `\\` | barra invertida |
| `\"` | aspas duplas |
| `\'` | aspas simples |
| `\0` | caractere nulo |

```ebnf
escape = "\\" ( "n" | "t" | "\\" | '"' | "'" | "0" ) ;
```

Qualquer outra barra invertida (`\q`) é erro léxico.

### 5.6 Lógico

`verdadeiro` e `falso` são palavras reservadas (seção 3); o lexer não emite um
token de literal lógico separado.

## 6. Operadores

Precedência e associatividade **não** são assunto do lexer — ficam na gramática
sintática (#14). Aqui só interessa quais sequências formam token.

### Aritméticos

| Token | Símbolo | Uso |
|---|---|---|
| `MAIS` | `+` | soma (e sinal unário) |
| `MENOS` | `-` | subtração (e sinal unário) |
| `VEZES` | `*` | multiplicação |
| `DIVIDIDO` | `/` | divisão |
| `RESTO` | `%` | resto da divisão inteira |

### Relacionais

| Token | Símbolo |
|---|---|
| `IGUAL` | `==` |
| `DIFERENTE` | `!=` |
| `MENOR` | `<` |
| `MENOR_IGUAL` | `<=` |
| `MAIOR` | `>` |
| `MAIOR_IGUAL` | `>=` |

### Lógicos

| Token | Símbolo | Uso |
|---|---|---|
| `E` | `&&` | conjunção |
| `OU` | `\|\|` | disjunção |
| `NAO` | `!` | negação (unário) |

### Bit a bit

| Token | Símbolo | Uso |
|---|---|---|
| `BIT_E` | `&` | conjunção bit a bit |
| `BIT_OU` | `\|` | disjunção bit a bit |
| `BIT_XOU` | `^` | ou exclusivo |
| `BIT_NAO` | `~` | complemento (unário) |
| `DESLOCA_ESQ` | `<<` | deslocamento à esquerda |
| `DESLOCA_DIR` | `>>` | deslocamento à direita |

Operam apenas sobre `inteiro`; a verificação é da análise semântica.

### Atribuição

| Token | Símbolo | Equivale a |
|---|---|---|
| `ATRIBUICAO` | `=` | — |
| `ATRIB_MAIS` | `+=` | `a = a + b` |
| `ATRIB_MENOS` | `-=` | `a = a - b` |
| `ATRIB_VEZES` | `*=` | `a = a * b` |
| `ATRIB_DIVIDIDO` | `/=` | `a = a / b` |
| `ATRIB_RESTO` | `%=` | `a = a % b` |
| `ATRIB_BIT_E` | `&=` | `a = a & b` |
| `ATRIB_BIT_OU` | `\|=` | `a = a \| b` |
| `ATRIB_BIT_XOU` | `^=` | `a = a ^ b` |
| `ATRIB_DESLOCA_ESQ` | `<<=` | `a = a << b` |
| `ATRIB_DESLOCA_DIR` | `>>=` | `a = a >> b` |

```ebnf
operador = "+"  | "-"  | "*"  | "/"  | "%"
         | "==" | "!=" | "<"  | "<=" | ">"  | ">="
         | "&&" | "||" | "!"
         | "&"  | "|"  | "^"  | "~"  | "<<" | ">>"
         | "="  | "+=" | "-=" | "*=" | "/=" | "%="
         | "&=" | "|=" | "^=" | "<<=" | ">>=" ;
```

### Armadilhas do casamento mais longo

Com os operadores compostos, vários prefixos passam a colidir. A regra da seção 2
resolve todos, mas vale ter em mente ao escrever o lexer e os testes:

| Entrada | Tokens | Não confundir com |
|---|---|---|
| `a&&b` | `a` `&&` `b` | `&` `&` |
| `a&=b` | `a` `&=` `b` | `&` `=` |
| `a>>=b` | `a` `>>=` `b` | `>>` `=` ou `>` `>=` |
| `a>>b` | `a` `>>` `b` | `>` `>` |

Ou seja: ao ver `>`, o lexer tenta `>>=`, depois `>>`, depois `>=`, e só então `>`.

Como `++` e `--` não existem neste dialeto, `a--b` são quatro tokens
(`a` `-` `-` `b`): o lexer aceita, e o parser resolve como `a - (-b)`.

## 7. Delimitadores

| Token | Símbolo | Uso |
|---|---|---|
| `ABRE_PAREN` | `(` | expressões, parâmetros |
| `FECHA_PAREN` | `)` | idem |
| `ABRE_CHAVE` | `{` | início de bloco |
| `FECHA_CHAVE` | `}` | fim de bloco |
| `ABRE_COLCHETE` | `[` | declaração e indexação de vetores |
| `FECHA_COLCHETE` | `]` | idem |
| `VIRGULA` | `,` | separador de argumentos e declarações |
| `PONTO_VIRGULA` | `;` | fim de comando |

```ebnf
delimitador = "(" | ")" | "{" | "}" | "[" | "]" | "," | ";" ;
```

> Para o lexer, `[` e `]` são apenas dois tokens. Como se declara um vetor
> (`inteiro v[10]`) e como se indexa (`v[i]`) é assunto da gramática sintática
> (#14).

## 8. Comentários

```ebnf
comentario       = comentario_linha | comentario_bloco ;
comentario_linha = "//" { qualquer_caractere - "\n" } ;
comentario_bloco = "/*" { qualquer_caractere } "*/" ;
```

- Comentário de bloco **não aninha**: o primeiro `*/` fecha.
- Bloco sem `*/` até o fim do arquivo é erro léxico.
- Comentários são descartados, mas as quebras de linha dentro deles continuam
  contando para a numeração de linhas.

## 9. Erros léxicos previstos

O lexer deve detectar pelo menos:

| Situação | Exemplo |
|---|---|
| Caractere não pertencente à linguagem | `@`, `#`, `$` |
| Cadeia não fechada na mesma linha | `"ola` |
| Caracter mal formado | `''`, `'ab'` |
| Escape desconhecido | `"\q"` |
| Número real mal formado | `3.`, `1e` |
| Comentário de bloco não fechado | `/* ...` até o fim do arquivo |
| Identificador começando com dígito | `2x` |

Todo erro carrega **linha e coluna** do ponto onde foi detectado. O formato exato
da mensagem é padronizado na issue #10.

## 10. Resumo dos tokens

| Categoria | Quantidade | Onde |
|---|---|---|
| Palavras reservadas | 19 | seção 3 |
| Identificador | 1 | seção 4 |
| Literais | 4 (inteiro, real, cadeia, caracter) | seção 5 |
| Operadores | 31 | seção 6 |
| Delimitadores | 8 | seção 7 |
| Controle | 1 (`FIM_ARQUIVO`) | seção 2 |

## 11. Exemplo comentado

Fonte:

```
// soma dois numeros
funcao inteiro soma(inteiro a, inteiro b) {
    retorne a + b;
}
```

Tokens produzidos (o comentário é descartado):

| # | Tipo | Lexema | Linha:Coluna |
|---|---|---|---|
| 1 | PALAVRA_RESERVADA | `funcao` | 2:1 |
| 2 | PALAVRA_RESERVADA | `inteiro` | 2:8 |
| 3 | IDENTIFICADOR | `soma` | 2:16 |
| 4 | ABRE_PAREN | `(` | 2:20 |
| 5 | PALAVRA_RESERVADA | `inteiro` | 2:21 |
| 6 | IDENTIFICADOR | `a` | 2:29 |
| 7 | VIRGULA | `,` | 2:30 |
| 8 | PALAVRA_RESERVADA | `inteiro` | 2:32 |
| 9 | IDENTIFICADOR | `b` | 2:40 |
| 10 | FECHA_PAREN | `)` | 2:41 |
| 11 | ABRE_CHAVE | `{` | 2:43 |
| 12 | PALAVRA_RESERVADA | `retorne` | 3:5 |
| 13 | IDENTIFICADOR | `a` | 3:13 |
| 14 | MAIS | `+` | 3:15 |
| 15 | IDENTIFICADOR | `b` | 3:17 |
| 16 | PONTO_VIRGULA | `;` | 3:18 |
| 17 | FECHA_CHAVE | `}` | 4:1 |
| 18 | FIM_ARQUIVO | — | 5:1 |

Linha e coluna começam em 1, e a coluna é contada em caracteres.
