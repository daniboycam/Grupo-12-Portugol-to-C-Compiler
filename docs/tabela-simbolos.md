# Tabela de Símbolos

A tabela de símbolos guarda os identificadores declarados no programa (variáveis, vetores, parâmetros e funções), com o tipo, a linha e o escopo de cada um. Ela é preenchida pelo parser durante a análise sintática e suporta escopos aninhados e sombreamento (*shadowing*).

## Estrutura

`include/portugol/tabela.hpp`

```cpp
enum class CategoriaSimbolo { VARIAVEL, PARAMETRO, FUNCAO };

struct Simbolo {
    std::string nome;
    std::string tipo;                           // "funcao" para funcoes
    int linha;
    CategoriaSimbolo categoria;
    int dimensao;                               // 0 para escalar, >0 para vetor
    std::string tipo_retorno;                   // apenas para funcoes
    std::vector<std::string> tipos_parametros;  // apenas para funcoes
    std::string escopo;                         // ex.: "global", "principal/para"
};
```

A classe `TabelaSimbolos` mantém uma pilha de escopos. O escopo global fica sempre na base e nunca é removido.

| Método | O que faz |
|---|---|
| `entrarEscopo(nome)` | empilha um escopo novo |
| `sairEscopo()` | desempilha o escopo atual |
| `nivel()` / `voltarAoNivel(n)` | profundidade da pilha / fecha escopos até a pilha ter `n` |
| `inserirSimbolo(...)` | insere no escopo atual; devolve `false` se o nome já existe **nesse mesmo escopo** |
| `buscarSimbolo(nome)` | procura do escopo atual para fora, respeitando o sombreamento |
| `buscarSimboloEscopoAtual(nome)` | procura só no escopo atual |
| `simbolos()` | todos os símbolos já inseridos, inclusive de escopos fechados |
| `imprimirTabela()` / `limpar()` | imprime a tabela completa / volta ao estado inicial |

## Escopos

| Construção | Escopo |
|---|---|
| Programa | `global`: variáveis globais e funções |
| `funcao f(...) { ... }` | `f`: parâmetros e corpo dividem o mesmo escopo |
| `para (...) { ... }` | `para`: a variável da inicialização e o corpo dividem o mesmo escopo |
| Bloco de `se`, `senao`, `enquanto` ou bloco solto | `bloco` |

O nome do escopo impresso é o caminho a partir do global, por exemplo `principal/para/bloco`.

Consequências:

- O mesmo nome pode ser declarado em funções diferentes, e parâmetros de funções diferentes não colidem.
- Dois `para (inteiro i = ...)` seguidos são válidos, porque cada `i` fica no escopo do próprio laço.
- Um escopo interno pode declarar um nome que já existe num escopo de fora (sombreamento).
- Redeclarar um nome no mesmo escopo é erro. Isso vale também para um parâmetro redeclarado no corpo da função, para a variável do `para` redeclarada no corpo do laço e para uma variável global com o mesmo nome de uma função.

A função entra na tabela no escopo de fora depois que o corpo é analisado, já com o tipo de retorno e os tipos dos parâmetros.

## Mensagem de erro

A redeclaração é relatada pelo parser com a linha da declaração nova e a da anterior:

```
erro semantico na linha 4: identificador 'y' ja declarado no mesmo escopo (linha 3)
```

## Exemplo

```
programa {
    inteiro total = 0;

    funcao real media(real a, real b) {
        real s = a + b;
        retorne s / 2;
    }

    funcao vazio principal() {
        inteiro notas[3];
        para (inteiro i = 0; i < 3; i += 1) {
            notas[i] = i * 10;
            se (notas[i] > 10) {
                inteiro total = notas[i];
                escreva(total, "\n");
            }
        }
        escreva(media(1.0, 2.0));
    }
}
```

```
===== Tabela de Simbolos =====
nome	categoria	tipo	escopo	linha
total	variavel	inteiro	global	2
a	parametro	real	media	4
b	parametro	real	media	4
s	variavel	real	media	5
media	funcao	real(real, real)	global	4
notas	variavel	inteiro[3]	principal	10
i	variavel	inteiro	principal/para	11
total	variavel	inteiro	principal/para/bloco	14
principal	funcao	vazio()	global	9
==============================
```

Vetores aparecem com a dimensão (`inteiro[3]`) e funções com a assinatura (`real(real, real)`). O `total` declarado dentro do `se` esconde o global, sem erro.
