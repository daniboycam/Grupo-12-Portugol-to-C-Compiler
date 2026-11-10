// Testes de integracao do fluxo lexer -> parser (issue #16).
//
// Diferente dos testes sintaticos, que passam o programa como string embutida,
// aqui cada caso e um arquivo .por de verdade, lido do disco. E o que exercita
// o caminho completo: abertura do arquivo, scanner alimentando o parser e o
// resultado final da analise.
#include <gtest/gtest.h>

#include <cstdio>
#include <stdexcept>
#include <string>

extern int yyparse();
extern void yyrestart(FILE* arquivo);
extern FILE* yyin;
extern int yylineno;
extern int yycolumn;
extern int yynerrs;

namespace {

struct ResultadoAnalise {
    int codigoRetorno;
    std::string mensagensErro;
};

// Caminho a partir da raiz do repositorio, que o CMake informa na compilacao.
std::string caminhoDe(const std::string& relativo) {
    return std::string(DIRETORIO_RAIZ) + "/" + relativo;
}

ResultadoAnalise analisarArquivo(const std::string& relativo) {
    const std::string caminho = caminhoDe(relativo);
    FILE* arquivo = std::fopen(caminho.c_str(), "r");
    if (arquivo == nullptr) {
        throw std::runtime_error("Nao foi possivel abrir " + caminho);
    }

    // O scanner guarda estado global, entao cada caso recomeca do zero. O
    // contador de erros do parser nao e zerado aqui de proposito: quem faz isso
    // e o proprio PortugolToC.y, e e isso que os testes de isolamento cobram.
    yyin = arquivo;
    yyrestart(arquivo);
    yylineno = 1;
    yycolumn = 1;

    testing::internal::CaptureStderr();
    const int codigo = yyparse();
    const std::string mensagens = testing::internal::GetCapturedStderr();

    std::fclose(arquivo);
    yyin = nullptr;
    return {codigo, mensagens};
}

struct Caso {
    const char* nome;
    const char* arquivo;
    const char* trechoEsperado;  // so usado nos casos invalidos
};

std::string nomeDoCaso(const testing::TestParamInfo<Caso>& info) {
    return info.param.nome;
}

// --- Programas que o fluxo completo deve aceitar ---

class ProgramasValidos : public testing::TestWithParam<Caso> {};

TEST_P(ProgramasValidos, AnalisaSemErro) {
    const ResultadoAnalise resultado = analisarArquivo(GetParam().arquivo);
    EXPECT_EQ(resultado.codigoRetorno, 0)
        << "Arquivo: " << GetParam().arquivo << "\nSaida: " << resultado.mensagensErro;
    EXPECT_EQ(resultado.mensagensErro, "")
        << "Nenhuma mensagem era esperada para " << GetParam().arquivo;
}

INSTANTIATE_TEST_SUITE_P(
    Pipeline, ProgramasValidos,
    testing::Values(
        Caso{"ProgramaCompleto", "tests/integracao/entradas/programa_completo.por", ""},
        Caso{"ComentariosELiterais", "tests/integracao/entradas/comentarios_e_literais.por", ""},
        Caso{"ExpressoesEPrecedencia", "tests/integracao/entradas/expressoes_precedencia.por", ""},
        Caso{"FinaisDeLinhaCrlf", "tests/integracao/entradas/finais_de_linha_crlf.por", ""},
        Caso{"ExemploCondicional", "examples/condicional.por", ""},
        Caso{"ExemploLacos", "examples/lacos.por", ""},
        Caso{"FixtureMinima", "tests/sintatico/entradas/valido_minimo.por", ""},
        Caso{"FixtureExpressao", "tests/sintatico/entradas/valido_expressao.por", ""}),
    nomeDoCaso);

// --- Programas que o fluxo completo deve recusar ---

class ProgramasInvalidos : public testing::TestWithParam<Caso> {};

TEST_P(ProgramasInvalidos, RecusaEExplicaOMotivo) {
    const ResultadoAnalise resultado = analisarArquivo(GetParam().arquivo);
    EXPECT_NE(resultado.codigoRetorno, 0)
        << "Esperava recusa para " << GetParam().arquivo;
    EXPECT_NE(resultado.mensagensErro.find(GetParam().trechoEsperado), std::string::npos)
        << "Arquivo: " << GetParam().arquivo << "\nSaida obtida: " << resultado.mensagensErro;
}

INSTANTIATE_TEST_SUITE_P(
    Pipeline, ProgramasInvalidos,
    testing::Values(
        // Erro do lexer que precisa chegar ate o resultado da analise.
        Caso{"CadeiaNaoFechada", "tests/integracao/entradas/invalido_cadeia_nao_fechada.por",
             "cadeia nao fechada"},
        Caso{"BlocoNaoFechado", "tests/integracao/entradas/invalido_bloco_nao_fechado.por",
             "fim de arquivo"},
        Caso{"LacoSemBloco", "tests/integracao/entradas/invalido_laco_sem_bloco.por",
             "inesperado 'inteiro'"},
        Caso{"ExpressaoIncompleta", "tests/sintatico/entradas/invalido_expressao.por",
             "inesperado ';'"},
        Caso{"DeclaracaoSemIdentificador", "tests/sintatico/entradas/invalido_sem_identificador.por",
             "inesperado ';'"}),
    nomeDoCaso);

// --- Um caso nao pode contaminar o proximo ---
//
// A regra programa usa "if (yynerrs > 0) YYABORT" para falhar quando houve
// recuperacao de erro. Como yynerrs e global e nao volta a zero sozinho entre
// chamadas do yyparse, sem o %initial-action do PortugolToC.y toda analise
// posterior a uma que falhou devolvia erro em silencio.

TEST(IsolamentoEntreAnalises, ProgramaValidoPassaDepoisDeUmInvalido) {
    const ResultadoAnalise invalido =
        analisarArquivo("tests/integracao/entradas/invalido_laco_sem_bloco.por");
    ASSERT_NE(invalido.codigoRetorno, 0);

    const ResultadoAnalise valido =
        analisarArquivo("tests/integracao/entradas/programa_completo.por");
    EXPECT_EQ(valido.codigoRetorno, 0)
        << "O estado global do parser vazou do caso anterior. Saida: "
        << valido.mensagensErro;
    EXPECT_EQ(valido.mensagensErro, "");
}

TEST(IsolamentoEntreAnalises, ErroLexicoEmEstadoExclusivoNaoVazaParaOProximo) {
    const ResultadoAnalise invalido =
        analisarArquivo("tests/integracao/entradas/invalido_cadeia_nao_fechada.por");
    ASSERT_NE(invalido.codigoRetorno, 0);

    const ResultadoAnalise valido =
        analisarArquivo("tests/integracao/entradas/comentarios_e_literais.por");
    EXPECT_EQ(valido.codigoRetorno, 0)
        << "O scanner continuou no estado de leitura de cadeia. Saida: "
        << valido.mensagensErro;
}

}  // namespace
