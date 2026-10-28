programa {
    inteiro i = 0;
    inteiro soma = 0;

    enquanto (i < 10) {
        soma += i;
        i += 1;
    }

    enquanto (verdadeiro) {
        se (soma > 100) {
            pare;
        } senao {
            soma += 10;
            continue;
        }
    }

    para (inteiro j = 0; j < 5; j += 1) {
        soma += j;
    }

    para (i = 0; i < 10; i = i + 1) {
        se (i == 5) {
            continue;
        }
    }

    para (; i < 20; ) {
        i += 2;
    }

    para (inteiro lin = 0; lin < 3; lin += 1) {
        inteiro col = 0;
        enquanto (col < 3) {
            se (lin == col) {
                pare;
            }
            col += 1;
        }
    }
}
