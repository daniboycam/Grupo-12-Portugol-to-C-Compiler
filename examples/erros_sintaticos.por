// Exemplo de recuperacao de erros sintaticos (Issue #33)
// O compilador identifica o erro no comando, emite o diagnostico e sincroniza
// no proximo ';' para continuar analisando o restante do programa.

programa {
    inteiro valido1 = 10;

    // Erro sintatico: atribuicao sem expressao
    inteiro erro1 = ;

    // O compilador se recupera e analisa o comando seguinte:
    inteiro valido2 = 20;

    // Outro erro sintatico: operador sem operando
    valido2 = + ;

    // Analise continua perfeitamente nos comandos e blocos seguintes:
    enquanto (valido1 > 0) {
        valido1 -= 1;
    }
}
