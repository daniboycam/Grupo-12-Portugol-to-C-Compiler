// Exemplo usado para exercitar o analisador lexico.
programa {
    funcao inteiro soma(inteiro a, inteiro b) {
        retorne a + b;
    }

    funcao vazio inicio() {
        inteiro x = 10;
        real taxa = 2.5e-3;
        cadeia nome = "Portugol";
        caracter inicial = 'P';
        logico ok = verdadeiro;

        se (x >= 10 && ok) {
            escreva(nome);
            x += soma(x, 1);
        } senao {
            escreva("menor");
        }

        enquanto (x > 0) {
            x -= 1;
        }
    }
}
