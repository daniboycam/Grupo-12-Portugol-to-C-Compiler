// Testes sintaticos para estruturas condicionais (issue #19).
#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>

extern int yyparse();
extern void yyrestart(FILE* arquivo);
extern FILE* yyin;
extern int yylineno;
extern int yycolumn;

namespace {

int analisar(const char* fonte) {
    FILE* arquivo = std::tmpfile();
    if (arquivo == nullptr) {
        throw std::runtime_error("Nao foi possivel criar arquivo temporario para o teste");
    }

    const size_t tamanho = std::strlen(fonte);
    if (std::fwrite(fonte, 1, tamanho, arquivo) != tamanho) {
        std::fclose(arquivo);
        throw std::runtime_error("Nao foi possivel escrever entrada do teste");
    }
    std::rewind(arquivo);

    // O scanner e o parser possuem estado global; reinicializa para cada caso.
    yyin = arquivo;
    yyrestart(arquivo);
    yylineno = 1;
    yycolumn = 1;

    const int resultado = yyparse();
    std::fclose(arquivo);
    yyin = nullptr;
    return resultado;
}

struct Caso {
    const char* nome;
    const char* fonte;
};

class CondicionaisValidas : public ::testing::TestWithParam<Caso> {};
TEST_P(CondicionaisValidas, ParserAceita) {
    const auto& caso = GetParam();
    EXPECT_EQ(analisar(caso.fonte), 0) << "Caso: " << caso.nome;
}

INSTANTIATE_TEST_SUITE_P(Condicional, CondicionaisValidas, ::testing::Values(
    Caso{"SeSimples", "programa { se (x > 5) { x = 1; } }"},
    Caso{"SeComSenao", "programa { se (x > 5) { x = 1; } senao { x = 2; } }"},
    Caso{"SenaoSeEncadeado", "programa { se (x == 1) { x = 2; } senao se (x == 2) { x = 3; } senao { x = 4; } }"},
    Caso{"SenaoSeLongo", "programa { se (x == 1) { x = 2; } senao se (x == 2) { x = 3; } senao se (x == 3) { x = 4; } senao se (x == 4) { x = 5; } }"},
    Caso{"SeAninhado", "programa { se (x > 0) { se (x < 10) { x = 1; } } }"},
    Caso{"SeAninhadoNoSenao", "programa { se (x > 0) { x = 1; } senao { se (x < 0) { x = 2; } senao { x = 3; } } }"},
    Caso{"CondicaoComposta", "programa { se (x > 0 && x < 10 || !ativo) { x = 1; } }"},
    Caso{"CondicaoComChamadaDeVetor", "programa { se (v[0] >= v[1]) { v[0] = 0; } }"},
    Caso{"DeclaracaoDentroDoBloco", "programa { se (x > 0) { inteiro y = 1; y += x; } }"},
    Caso{"BlocoVazio", "programa { se (x > 0) { } senao { } }"},
    Caso{"SeDentroDeLaco", "programa { enquanto (x > 0) { se (x == 5) { pare; } x -= 1; } }"},
    Caso{"LacoDentroDoSe", "programa { se (ativo) { para (i = 0; i < 3; i += 1) { x += i; } } }"}
), [](const ::testing::TestParamInfo<Caso>& info) {
    return std::string(info.param.nome);
});

class CondicionaisInvalidas : public ::testing::TestWithParam<Caso> {};
TEST_P(CondicionaisInvalidas, ParserRejeita) {
    const auto& caso = GetParam();
    EXPECT_NE(analisar(caso.fonte), 0) << "Caso: " << caso.nome;
}

INSTANTIATE_TEST_SUITE_P(Condicional, CondicionaisInvalidas, ::testing::Values(
    Caso{"SemParenteses", "programa { se x > 5 { x = 1; } }"},
    Caso{"SemBloco", "programa { se (x > 5) x = 1; }"},
    Caso{"SenaoSemBloco", "programa { se (x > 5) { x = 1; } senao x = 2; }"},
    Caso{"SenaoSolto", "programa { senao { x = 1; } }"},
    Caso{"CondicaoVazia", "programa { se () { x = 1; } }"},
    Caso{"ParenteseNaoFechado", "programa { se (x > 5 { x = 1; } }"},
    Caso{"BlocoNaoFechado", "programa { se (x > 5) { x = 1; }"},
    Caso{"DoisSenaoSeguidos", "programa { se (x > 5) { x = 1; } senao { x = 2; } senao { x = 3; } }"}
), [](const ::testing::TestParamInfo<Caso>& info) {
    return std::string(info.param.nome);
});

}  // namespace
