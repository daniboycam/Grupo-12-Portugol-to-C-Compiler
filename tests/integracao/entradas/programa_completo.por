// Programa que exercita o lexer e o parser juntos.
programa {
    inteiro total = 0;
    inteiro limite = 10;
    cadeia rotulo = "contagem";

    /* Comentario de bloco
       espalhado por varias linhas, no meio das declaracoes. */
    para (inteiro i = 0; i < limite; i += 1) {
        se (i % 2 == 0) {
            total += i;
        } senao se (i == 7) {
            continue;
        } senao {
            total -= 1;
        }
    }

    enquanto (total > limite) {
        total = total / 2;
        se (total == 0) {
            pare;
        }
    }
}
