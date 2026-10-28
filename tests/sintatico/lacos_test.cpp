// Testes sintaticos para lacos de repeticao e desvios incondicionais (issue #30).
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

class LacosValidos : public ::testing::TestWithParam<Caso> {};
TEST_P(LacosValidos, ParserAceita) {
    const auto& caso = GetParam();
    EXPECT_EQ(analisar(caso.fonte), 0) << "Caso: " << caso.nome;
}

INSTANTIATE_TEST_SUITE_P(Repeticao, LacosValidos, ::testing::Values(
    Caso{"EnquantoSimples", "programa { enquanto (x < 10) { x += 1; } }"},
    Caso{"EnquantoComposto", "programa { enquanto (x > 0 && ativo) { x -= 1; } }"},
    Caso{"EnquantoComPareEContinue", "programa { enquanto (verdadeiro) { se (x == 5) { pare; } senao { x += 1; continue; } } }"},
    Caso{"EnquantoAninhado", "programa { enquanto (i < 10) { enquanto (j < 10) { j += 1; } i += 1; } }"},
    Caso{"ParaCompleto", "programa { para (i = 0; i < 10; i += 1) { x += i; } }"},
    Caso{"ParaComDeclaracao", "programa { para (inteiro i = 0; i < 10; i = i + 1) { x += i; } }"},
    Caso{"ParaSemInicializacao", "programa { para (; i < 10; i += 1) { x += i; } }"},
    Caso{"ParaSemPasso", "programa { para (i = 0; i < 10; ) { i += 1; } }"},
    Caso{"ParaApenasCondicao", "programa { para (; x < 10; ) { x += 1; } }"},
    Caso{"ParaInfinitoVazio", "programa { para (;;) { pare; } }"},
    Caso{"ParaAninhado", "programa { para (i = 0; i < 5; i += 1) { para (j = 0; j < 5; j += 1) { pare; } } }"},
    Caso{"LacosMistos", "programa { para (i = 0; i < 5; i += 1) { enquanto (x > 0) { x -= 1; } } }"},
    Caso{"PareEContinueAvulsos", "programa { pare; continue; }"},
    Caso{"BlocoInternoIsolado", "programa { { inteiro x = 1; } }"}
), [](const ::testing::TestParamInfo<Caso>& info) {
    return std::string(info.param.nome);
});

class LacosInvalidos : public ::testing::TestWithParam<Caso> {};
TEST_P(LacosInvalidos, ParserRejeita) {
    const auto& caso = GetParam();
    EXPECT_NE(analisar(caso.fonte), 0) << "Caso: " << caso.nome;
}

INSTANTIATE_TEST_SUITE_P(Repeticao, LacosInvalidos, ::testing::Values(
    Caso{"EnquantoSemParenteses", "programa { enquanto x < 10 { x += 1; } }"},
    Caso{"EnquantoSemFechaParenteses", "programa { enquanto (x < 10 { x += 1; } }"},
    Caso{"EnquantoSemCondicao", "programa { enquanto () { x += 1; } }"},
    Caso{"EnquantoSemBloco", "programa { enquanto (x < 10) x += 1; }"},
    Caso{"ParaSemParenteses", "programa { para i = 0; i < 10; i += 1 { } }"},
    Caso{"ParaSemPontoVirgula1", "programa { para (i = 0 i < 10; i += 1) { } }"},
    Caso{"ParaSemPontoVirgula2", "programa { para (i = 0; i < 10 i += 1) { } }"},
    Caso{"ParaSemBloco", "programa { para (i = 0; i < 10; i += 1) x += 1; }"},
    Caso{"PareSemPontoVirgula", "programa { pare }"},
    Caso{"ContinueSemPontoVirgula", "programa { continue }"}
), [](const ::testing::TestParamInfo<Caso>& info) {
    return std::string(info.param.nome);
});

} // namespace
