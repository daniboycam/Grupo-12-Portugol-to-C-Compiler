// Demonstracao ponta a ponta do compilador: usa os principais recursos da
// linguagem (globais, recursao, vetor, lacos, desvios, leia/escreva,
// conversao caracter -> cadeia e variaveis escondidas em blocos internos).
//
//   ./build/portugolc -o demonstracao.cpp examples/demonstracao.por
//   g++ -std=c++17 -Wall -Wextra -Wpedantic demonstracao.cpp -o demonstracao
//   echo "Ana 6" | ./demonstracao
programa {
    inteiro chamadas = 0;
    real PI = 3.14159;

    funcao inteiro fatorial(inteiro n) {
        chamadas += 1;
        se (n <= 1) { retorne 1; }
        retorne n * fatorial(n - 1);
    }

    funcao logico ehPrimo(inteiro n) {
        se (n < 2) { retorne falso; }
        para (inteiro d = 2; d * d <= n; d += 1) {
            se (n % d == 0) { retorne falso; }
        }
        retorne verdadeiro;
    }

    funcao cadeia saudacao(cadeia nome, caracter fim) {
        retorne "Ola, " + nome + fim;
    }

    funcao real area(real raio) {
        retorne PI * raio * raio;
    }

    funcao vazio principal() {
        inteiro n;
        cadeia nome;
        leia(nome);
        leia(n);

        escreva(saudacao(nome, '!'), "\n");
        escreva("fatorial(", n, ") = ", fatorial(n), " em ", chamadas, " chamadas\n");

        inteiro v[10];
        inteiro qtd = 0;
        para (inteiro i = 2; i < 30 && qtd < 10; i += 1) {
            se (ehPrimo(i)) { v[qtd] = i; qtd += 1; }
        }
        escreva("primos:");
        para (inteiro i = 0; i < qtd; i += 1) { escreva(" ", v[i]); }
        escreva("\n");

        inteiro soma = 0;
        inteiro i = 0;
        enquanto (verdadeiro) {
            i += 1;
            se (i > 20) { pare; }
            se (i % 2 == 1) { continue; }
            soma += i;
        }
        escreva("soma dos pares ate 20 = ", soma, "\n");

        escreva("7/2=", 7 / 2, " 7.0/2=", 7.0 / 2, " 7%3=", 7 % 3, "\n");
        escreva("bits: ", 6 & 3, " ", 6 | 3, " ", 6 ^ 3, " ", 1 << 5, " ", 64 >> 2, " ", ~0, "\n");
        escreva("logico: ", 3 > 2 && !(1 == 2), " ", 'a' < 'b', " ", "abc" == "abc", "\n");
        escreva("area(2) = ", area(2), "\n");

        cadeia s = 'x';
        s += 'y';
        se (s == "xy") {
            cadeia s = s + "z";
            escreva("sombreado: ", s, "\n");
        }
        escreva("externo: ", s, "\n");

        inteiro x = 1;
        se (x == 1) { inteiro x = x + 10; escreva("x interno = ", x, "\n"); }
        escreva("x externo = ", x, "\n");
    }
}
