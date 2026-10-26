# Gramática sintática do Portugol — Grupo 12 (EBNF)

**Issue:** [#14 — Gramática livre de contexto do Portugol](https://github.com/daniboycam/Grupo-12-Portugol-to-C-Compiler/issues/14)
**Finalidade:** especificar a sintaxe da linguagem que orientará as produções do Bison, os testes do parser e a construção da AST.
**Referência léxica:** `docs/gramatica-lexica.md` na branch `devel`.

> **Status:** proposta inicial para revisão do grupo. O lexer já reconhece os terminais aqui utilizados, mas a maior parte destas produções **ainda não está implementada** em `src/PortugolToC.y`. Este documento descreve a linguagem-alvo, não funcionalidades já testadas.

## 1. Convenções

- As palavras reservadas são escritas **sem acentos** e em minúsculas, como `funcao`, `logico`, `senao`, `leia` e `escreva`.
- Terminais entre aspas representam exatamente os caracteres reconhecidos pelo Flex (ex.: `"funcao"`, `"("`, `"=="`).
- `IDENTIFICADOR`, `NUM_INT`, `NUM_REAL`, `LIT_STRING` e `LIT_CHAR` são terminais léxicos (tokens), não palavras literais escritas no código-fonte.
- EBNF: `A | B` significa alternativa; `[ A ]`, opcional; `{ A }`, repetição de zero ou mais; `( A )`, agrupamento; e `;`, fim da produção **da metalinguagem**.
- Espaços e comentários são ignorados pelo lexer. O fim do arquivo é tratado pelo parser.

## 2. Estrutura geral do programa

Há dois formatos aceitos **pela gramática proposta**: um bloco `programa { ... }` e um conjunto de declarações de variáveis/funções diretamente no topo do arquivo. O segundo formato preserva o exemplo de função isolada já presente na documentação léxica. A equipe deve confirmar ambos antes da implementação definitiva.

~~~ebnf
arquivo                = programa_delimitado | unidade_topo, { unidade_topo } ;
programa_delimitado    = "programa", "{", { unidade_topo | comando_execucao }, "}" ;
unidade_topo           = declaracao_variavel | definicao_funcao ;

definicao_funcao       = "funcao", tipo_retorno, IDENTIFICADOR,
                         "(", [ lista_parametros ], ")", bloco ;
lista_parametros       = parametro, { ",", parametro } ;
parametro              = tipo, IDENTIFICADOR ;
tipo_retorno           = tipo | "vazio" ;
tipo                   = "inteiro" | "real" | "cadeia" | "caracter" | "logico" ;

bloco                  = "{", { comando }, "}" ;
comando                = declaracao_variavel | comando_execucao ;
~~~

**Observações:** `principal` é um `IDENTIFICADOR` comum, pois não é palavra reservada. A existência e a assinatura de `funcao vazio principal()` são requisitos a verificar na fase semântica/estrutura do programa (issue #31). `vazio` não é permitido como tipo de variável.

## 3. Declarações de variáveis

~~~ebnf
declaracao_variavel    = tipo, declarador, { ",", declarador }, ";" ;
declarador             = IDENTIFICADOR, [ "[", NUM_INT, "]" ],
                         [ "=", expressao ] ;
~~~

Exemplos de sintaxe: `inteiro idade;`, `real x = 2.5;`, `inteiro a, b;` e `inteiro valores[10];`. Um tamanho de vetor positivo, identificadores duplicados e compatibilidade de tipos são verificações **semânticas**, não exigências da gramática.

## 4. Comandos

~~~ebnf
comando_execucao      = comando_atribuicao
                      | comando_chamada
                      | comando_leitura
                      | comando_escrita
                      | comando_retorno
                      | comando_condicional
                      | comando_enquanto
                      | comando_para
                      | "pare", ";"
                      | "continue", ";"
                      | bloco ;

comando_atribuicao    = acesso, operador_atribuicao, expressao, ";" ;
comando_chamada       = chamada_funcao, ";" ;
comando_leitura       = "leia", "(", acesso, ")", ";" ;
comando_escrita       = "escreva", "(", lista_argumentos, ")", ";" ;
comando_retorno       = "retorne", [ expressao ], ";" ;

comando_condicional   = "se", "(", expressao, ")", bloco,
                         [ "senao", ( bloco | comando_condicional ) ] ;
comando_enquanto      = "enquanto", "(", expressao, ")", bloco ;
comando_para          = "para", "(", [ atribuicao_sem_ponto_virgula ], ";",
                         [ expressao ], ";", [ atribuicao_sem_ponto_virgula ],
                         ")", bloco ;
atribuicao_sem_ponto_virgula = acesso, operador_atribuicao, expressao ;

chamada_funcao        = IDENTIFICADOR, "(", [ lista_argumentos ], ")" ;
lista_argumentos      = expressao, { ",", expressao } ;
acesso                = IDENTIFICADOR, [ "[", expressao, "]" ] ;
~~~

Regras e exemplos importantes:

- `leia(x);` recebe exatamente **um destino atribuível** (variável ou posição de vetor). `leia();` e `leia(x, y);` são sintaticamente inválidos nesta versão.
- `escreva("Total: ", valor + 10, "\n");` aceita **um ou mais** argumentos, separados por vírgula. `escreva();` é inválido, conforme o critério de aceite da issue #32.
- `retorne;` é sintaticamente permitido; se é válido para determinada função depende de o retorno ser `vazio` (issue #31).
- `pare;` e `continue;` só devem ocorrer em laços; isso é verificação de contexto, não desta gramática.
- A gramática usa blocos `{ ... }` nas condicionais, e foi essa a forma implementada. A issue #19 descrevia uma variante `se ... entao ... fimse`, mas `entao` e `fimse` não existem nos tokens do lexer, então o grupo optou por manter as chaves, coerentes com o resto do dialeto. O texto da issue ficou desatualizado.
- Como o bloco é obrigatório nos dois lados, o **dangling-else não ocorre**: quando o parser encontra o `senao`, o bloco anterior já foi fechado pela chave. A gramática gera zero conflitos de shift/reduce, sem precisar de precedência para isso.
- O `senao` aceita um bloco ou **outra condicional**, o que permite encadear `se ... senao se ... senao`. Sem isso seria preciso aninhar um bloco a cada condição nova.

## 5. Expressões e precedência

A cadeia de produções abaixo torna a **precedência explícita**. Repetições `{ operador, próximo_nível }` associam os operadores binários à esquerda; a atribuição é à direita; os prefixos unários são aplicados do último para o primeiro. Os operadores correspondem aos tokens já existentes no `PortugolToC.y`.

~~~ebnf
expressao             = expressao_ou, [ operador_atribuicao, expressao ] ;
expressao_ou          = expressao_e, { "||", expressao_e } ;
expressao_e           = expressao_bit_ou, { "&&", expressao_bit_ou } ;
expressao_bit_ou      = expressao_bit_xou, { "|", expressao_bit_xou } ;
expressao_bit_xou     = expressao_bit_e, { "^", expressao_bit_e } ;
expressao_bit_e       = expressao_igualdade, { "&", expressao_igualdade } ;
expressao_igualdade   = expressao_relacional,
                         { ( "==" | "!=" ), expressao_relacional } ;
expressao_relacional  = expressao_deslocamento,
                         { ( "<" | "<=" | ">" | ">=" ), expressao_deslocamento } ;
expressao_deslocamento = expressao_aditiva,
                         { ( "<<" | ">>" ), expressao_aditiva } ;
expressao_aditiva     = expressao_multiplicativa,
                         { ( "+" | "-" ), expressao_multiplicativa } ;
expressao_multiplicativa = expressao_unaria,
                         { ( "*" | "/" | "%" ), expressao_unaria } ;
expressao_unaria      = { "+" | "-" | "!" | "~" }, expressao_primaria ;
expressao_primaria    = NUM_INT | NUM_REAL | LIT_STRING | LIT_CHAR
                      | "verdadeiro" | "falso"
                      | chamada_funcao | acesso | "(", expressao, ")" ;

operador_atribuicao   = "=" | "+=" | "-=" | "*=" | "/=" | "%=" |
                         "&=" | "|=" | "^=" | "<<=" | ">>=" ;
~~~

### Ordem de precedência (da menor para a maior)

| Nível | Operadores | Associatividade |
|---|---|---|
| 1 | `=`, `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `\|=`, `^=`, `<<=`, `>>=` | Direita |
| 2 | `\|\|` | Esquerda |
| 3 | `&&` | Esquerda |
| 4 | `\|` | Esquerda |
| 5 | `^` | Esquerda |
| 6 | `&` | Esquerda |
| 7 | `==`, `!=` | Esquerda |
| 8 | `<`, `<=`, `>`, `>=` | Esquerda |
| 9 | `<<`, `>>` | Esquerda |
| 10 | `+`, `-` binários | Esquerda |
| 11 | `*`, `/`, `%` | Esquerda |
| 12 | `+`, `-`, `!`, `~` unários | Direita (prefixo) |

**Importante:** a expressão genérica permite uma atribuição com lado esquerdo que não é variável, por exemplo `2 = 3`; o parser ou a análise semântica deve rejeitar essa situação. Para evitar aceitação indevida, a produção de **comando de atribuição** já exige `acesso` à esquerda. As precedências que já existem no arquivo Bison devem ser ajustadas para contemplar também `+` e `-` unários.

## 6. Exemplos para orientar testes do parser

### 6.1 Programa com cabeçalho e comandos básicos

~~~portugol
programa {
    inteiro x;
    x = 2 + 3 * 4;
    escreva("Resultado: ", x);
}
~~~

### 6.2 Funções no topo do arquivo

~~~portugol
funcao inteiro soma(inteiro a, inteiro b) {
    retorne a + b;
}

funcao vazio principal() {
    inteiro valor;
    leia(valor);
    escreva("Total: ", soma(valor, 10), "\n");
    retorne;
}
~~~

### 6.3 Entradas que o parser deverá rejeitar

~~~portugol
inteiro ;             // falta identificador
leia();               // falta destino
escreva();            // falta argumento
funcao inteiro f( {   // parâmetros e delimitadores inválidos
~~~

Esses exemplos **não foram executados** contra o parser atual, que ainda não contém as produções necessárias. São casos de teste propostos para as issues #17, #18, #31 e #32.

## 7. Correspondência com o Bison e próximos passos

| Regras desta gramática | Tokens / ponto de integração | Issue(s) |
|---|---|---|
| Estrutura, declarações, expressões | `KW_PROGRAMA`, `KW_INTEIRO`, `IDENTIFICADOR`, operadores | #18 |
| Testes de entradas válidas e inválidas | `yyparse()` e testes de integração Flex+Bison | #17 |
| `leia`, `escreva` e argumentos | `KW_LEIA`, `KW_ESCREVA`, `VIRGULA` | #32 |
| Funções, parâmetros e `retorne` | `KW_FUNCAO`, `KW_RETORNE`, `KW_VAZIO` | #31 |
| Condicionais e repetições | `KW_SE`, `KW_SENAO`, `KW_ENQUANTO`, `KW_PARA` | #19, #30 |
| Nós de expressões, declarações e funções | Ações semânticas no `PortugolToC.y` | #36 |

**Acordos pendentes para revisão da equipe:**

1. Confirmar a aceitação dos dois formatos de arquivo (com ou sem `programa { ... }`).
2. Decidir se condicionais terão somente blocos `{ ... }` ou se o grupo vai adicionar os tokens `entao` / `fimse` requeridos pela descrição da issue #19.
3. Definir se vetores, inicializadores e comandos `para` opcionais integram o escopo da P2.
4. Confirmar se `principal` é obrigatória e onde a validação dessa regra acontecerá.

Concluídos esses alinhamentos, este documento pode ser considerado a especificação de referência para a implementação das produções no Bison.
