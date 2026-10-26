// Condicionais aceitas pelo analisador sintatico.
programa {
    inteiro nota = 75;
    cadeia conceito = "";

    se (nota >= 90) {
        conceito = "A";
    } senao se (nota >= 80) {
        conceito = "B";
    } senao se (nota >= 70) {
        conceito = "C";
    } senao {
        conceito = "reprovado";
    }

    se (nota > 0 && nota <= 100) {
        se (nota % 2 == 0) {
            nota = nota / 2;
        }
    }
}
