# Demonstração ponta a ponta

Passo a passo para mostrar o compilador funcionando, do programa Portugol ao executável. Todos os comandos partem da raiz do repositório.

## 1. Compilar o compilador e rodar os testes

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

```
100% tests passed, 0 tests failed out of 467
```

## 2. Traduzir, compilar e executar um programa

O [examples/demonstracao.por](../examples/demonstracao.por) usa os principais recursos da linguagem: variáveis globais, recursão, vetor, `para`, `enquanto` com `pare` e `continue`, `leia`/`escreva`, operadores aritméticos, lógicos e bit a bit, conversão de `caracter` para `cadeia` e variáveis escondidas em blocos internos.

```bash
./build/portugolc -o demonstracao.cpp examples/demonstracao.por
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror demonstracao.cpp -o demonstracao
echo "Ana 6" | ./demonstracao
```

```
Ola, Ana!
fatorial(6) = 720 em 6 chamadas
primos: 2 3 5 7 11 13 17 19 23 29
soma dos pares ate 20 = 110
7/2=3 7.0/2=3.5 7%3=1
bits: 2 7 5 32 16 -1
logico: verdadeiro verdadeiro verdadeiro
area(2) = 12.5664
sombreado: xyz
externo: xy
x interno = 11
x externo = 1
```

Com `-o -` o C++ sai na tela. Trecho da tradução da função recursiva:

```
funcao inteiro fatorial(inteiro n) {        int p_fatorial(int p_n) {
    chamadas += 1;                              p_chamadas += 1;
    se (n <= 1) { retorne 1; }        →         if (p_n <= 1) {
    retorne n * fatorial(n - 1);                    return 1;
}                                               }
                                                return p_n * p_fatorial(p_n - 1);
                                            }
```

## 3. Ver as fases intermediárias

Tokens reconhecidos pelo analisador léxico:

```bash
./build/portugolc --tokens examples/exemplo.por
```

Tabela de símbolos (com categoria e escopo) e AST, sem gerar código:

```bash
./build/portugolc examples/condicional.por
```

## 4. Erros de cada fase

Cada fase recusa o programa com a linha do problema, sai com código `1` e não gera `.cpp`.

**Léxico:**

```bash
printf 'programa {\n inteiro x = 1; @\n}\n' | ./build/portugolc -o -
```

```
Erro léxico [Linha 2, Coluna 17]: caractere inesperado '@'
Falha na analise lexica (1 erro(s)).
```

**Sintático:**

```bash
printf 'programa {\n inteiro x = ;\n}\n' | ./build/portugolc -o -
```

```
Erro sintático na linha 2: inesperado ';'
Falha na analise sintatica.
```

**Semântico** (todos os erros aparecem de uma vez):

```bash
cat > erros.por <<'EOF'
programa {
    funcao inteiro dobro(inteiro x) { retorne x * 2; }
    funcao vazio principal() {
        inteiro a = "texto";
        escreva(naoExiste);
        dobro(1, 2);
        pare;
        logico b = 1 + verdadeiro;
    }
}
EOF
./build/portugolc -o erros.cpp erros.por
```

```
Erro semântico na linha 4: nao e possivel inicializar 'a' (inteiro) com um valor do tipo cadeia
Erro semântico na linha 5: variavel 'naoExiste' nao declarada
Erro semântico na linha 6: funcao 'dobro' espera 1 argumento(s), mas recebeu 2
Erro semântico na linha 7: 'pare' so pode ser usado dentro de um laco (enquanto/para)
Erro semântico na linha 8: operador '+' nao se aplica aos tipos inteiro e logico
Falha na analise semantica (5 erro(s)).
```

**Redeclaração no mesmo escopo** (acusada pela tabela de símbolos):

```bash
printf 'programa {\n inteiro x;\n inteiro x;\n}\n' | ./build/portugolc -o -
```

```
Erro semântico na linha 3: identificador 'x' ja declarado no mesmo escopo (linha 2)
Falha na analise semantica (1 erro(s)).
```
