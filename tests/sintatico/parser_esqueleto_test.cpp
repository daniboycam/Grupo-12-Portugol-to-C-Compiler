// Testes do esqueleto sintatico (issue #17).
// Cada caso invoca o parser Bison usando a entrada entregue pelo Flex.
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

    // O scanner e o parser possuem estado global; reinicialize para cada caso.
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

class ProgramasValidos : public ::testing::TestWithParam<Caso> {};
TEST_P(ProgramasValidos, ParserAceita) {
    const auto& caso = GetParam();
    EXPECT_EQ(analisar(caso.fonte), 0) << "Caso: " << caso.nome;
}

INSTANTIATE_TEST_SUITE_P(Esqueleto, ProgramasValidos, ::testing::Values(
    Caso{"BlocoVazio", "programa {}"},
    Caso{"DeclaracaoSimples", "programa { inteiro x; }"},
    Caso{"DeclaracoesNoTopo", "inteiro x; real y = 2.0;"},
    Caso{"MultiplasVariaveis", "programa { inteiro a, b, c; }"},
    Caso{"InicializacaoReal", "programa { real taxa = 2.5e-3; }"},
    Caso{"InicializacaoCadeiaCaracter", "programa { cadeia nome = \"ola\"; caracter letra = 'P'; }"},
    Caso{"InicializacaoLogica", "programa { logico ativo = verdadeiro && !falso; }"},
    Caso{"VetorEIndexacao", "programa { inteiro v[10]; v[2] = 5; }"},
    Caso{"Aritmetica", "programa { inteiro x = 2 + 3 * 4; }"},
    Caso{"UnariosEParenteses", "programa { inteiro x = -4 + (+2); }"},
    Caso{"AtribuicaoComposta", "programa { inteiro x = 10; x += 5; }"},
    Caso{"ComparacaoELogica", "programa { inteiro x = 10; logico ok = x >= 2 && x != 0; }"},
    Caso{"BitsEDeslocamento", "programa { inteiro x = 3; x = (x << 1) | 2; }"},
    Caso{"Comentarios", "// cabecalho\nprograma { /* variavel */ inteiro x; }"}
), [](const ::testing::TestParamInfo<Caso>& info) {
    return std::string(info.param.nome);
});

class ProgramasInvalidos : public ::testing::TestWithParam<Caso> {};
TEST_P(ProgramasInvalidos, ParserRejeita) {
    const auto& caso = GetParam();
    EXPECT_NE(analisar(caso.fonte), 0) << "Caso: " << caso.nome;
}

INSTANTIATE_TEST_SUITE_P(Esqueleto, ProgramasInvalidos, ::testing::Values(
    Caso{"VariavelSemNome", "programa { inteiro ; }"},
    Caso{"DeclaracaoSemPontoVirgula", "programa { inteiro x }"},
    Caso{"InicializacaoSemExpressao", "programa { inteiro x = ; }"},
    Caso{"DeclaradoresComVirgulaExtra", "programa { inteiro a,; }"},
    Caso{"VetorSemTamanho", "programa { inteiro v[]; }"},
    Caso{"VetorSemFechamento", "programa { inteiro v[10; }"},
    Caso{"AtribuicaoSemExpressao", "programa { x = ; }"},
    Caso{"ParentesesSemFechamento", "programa { x = (1 + 2; }"},
    Caso{"OperadorSemOperando", "programa { x = +; }"},
    Caso{"BlocoSemFechamento", "programa { inteiro x = 2; "},
    Caso{"PontoVirgulaAusente", "programa { inteiro x = 1 }"},
    Caso{"ChaveExtra", "programa { inteiro x = 2; } }"},
    Caso{"NumerosConsecutivos", "programa { inteiro x = 3 4; }"}
), [](const ::testing::TestParamInfo<Caso>& info) {
    return std::string(info.param.nome);
});

} // namespace
