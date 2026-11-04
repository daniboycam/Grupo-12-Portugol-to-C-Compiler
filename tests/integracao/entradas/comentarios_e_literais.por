programa {
    // Literais com escape e acentuacao convivendo com o parser.
    cadeia mensagem = "linha 1\nlinha 2\tfim";
    cadeia acentuada = "programacao e compilacao";
    cadeia aspas = "ele disse \"ola\"";
    caracter inicial = 'P';
    caracter quebra = '\n';
    logico ativo = verdadeiro;
    real taxa = 2.5e-3;

    inteiro x = /* comentario no meio da expressao */ 40 + 2;

    se (ativo) {
        x = x + 1;  // comentario depois do comando
    }
}
