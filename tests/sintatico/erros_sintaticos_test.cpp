// Testes sintaticos para mensagens de erro e recuperacao com 'error ;' (issue #33).
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

struct ResultadoAnalise {
    int codigoRetorno;
    std::string mensagensErro;
};

ResultadoAnalise analisarComCaptura(const char* fonte) {
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

    // Reinicializa o estado global do scanner e do parser
    yyin = arquivo;
    yyrestart(arquivo);
    yylineno = 1;
    yycolumn = 1;

    testing::internal::CaptureStderr();
    const int resultado = yyparse();
    const std::string saida = testing::internal::GetCapturedStderr();

    std::fclose(arquivo);
    yyin = nullptr;
    return {resultado, saida};
}

TEST(MensagensDeErroSintatico, LinhaETokenInesperadoDelimitador) {
    const char* fonte =
        "programa {\n"
        "    inteiro x = ;\n"
        "}\n";
    auto [res, err] = analisarComCaptura(fonte);
    EXPECT_NE(res, 0);
    EXPECT_NE(err.find("Erro sintático na linha 2: inesperado ';'"), std::string::npos)
        << "Saida obtida: " << err;
}

TEST(MensagensDeErroSintatico, LinhaETokenInesperadoNumero) {
    const char* fonte =
        "programa {\n"
        "    inteiro 123;\n"
        "}\n";
    auto [res, err] = analisarComCaptura(fonte);
    EXPECT_NE(res, 0);
    EXPECT_NE(err.find("Erro sintático na linha 2: inesperado '123'"), std::string::npos)
        << "Saida obtida: " << err;
}

TEST(MensagensDeErroSintatico, LinhaETokenInesperadoPalavraReservada) {
    const char* fonte =
        "programa {\n"
        "    inteiro se;\n"
        "}\n";
    auto [res, err] = analisarComCaptura(fonte);
    EXPECT_NE(res, 0);
    EXPECT_NE(err.find("Erro sintático na linha 2: inesperado 'se'"), std::string::npos)
        << "Saida obtida: " << err;
}

TEST(MensagensDeErroSintatico, LinhaEFimDeArquivoInesperado) {
    const char* fonte =
        "programa {\n"
        "    inteiro x = 5";
    auto [res, err] = analisarComCaptura(fonte);
    EXPECT_NE(res, 0);
    EXPECT_NE(err.find("Erro sintático na linha 2: inesperado 'fim de arquivo'"), std::string::npos)
        << "Saida obtida: " << err;
}

TEST(RecuperacaoDeErros, SincronizaAposPontoEVirgulaEMultiplosComandos) {
    const char* fonte =
        "programa {\n"
        "    inteiro a = ;\n"
        "    inteiro b = 10;\n"
        "    c = ;\n"
        "    inteiro d = 20;\n"
        "}\n";
    auto [res, err] = analisarComCaptura(fonte);
    EXPECT_NE(res, 0);
    EXPECT_NE(err.find("Erro sintático na linha 2: inesperado ';'"), std::string::npos)
        << "Saida obtida: " << err;
    EXPECT_NE(err.find("Erro sintático na linha 4: inesperado ';'"), std::string::npos)
        << "Saida obtida: " << err;
}

TEST(RecuperacaoDeErros, SincronizaDentroDeLacosEComandosSeguintes) {
    const char* fonte =
        "programa {\n"
        "    x = 1 + ;\n"
        "    enquanto (x < 10) {\n"
        "        y = + ;\n"
        "        x += 1;\n"
        "    }\n"
        "    z = ;\n"
        "}\n";
    auto [res, err] = analisarComCaptura(fonte);
    EXPECT_NE(res, 0);
    EXPECT_NE(err.find("Erro sintático na linha 2: inesperado ';'"), std::string::npos)
        << "Saida obtida: " << err;
    EXPECT_NE(err.find("Erro sintático na linha 4: inesperado ';'"), std::string::npos)
        << "Saida obtida: " << err;
    EXPECT_NE(err.find("Erro sintático na linha 7: inesperado ';'"), std::string::npos)
        << "Saida obtida: " << err;
}

TEST(RecuperacaoDeErros, ProgramaTotalmenteValidoNaoEmiteErros) {
    const char* fonte =
        "programa {\n"
        "    inteiro x = 10;\n"
        "    x += 5;\n"
        "    se (x > 10) {\n"
        "        x = 0;\n"
        "    }\n"
        "}\n";
    auto [res, err] = analisarComCaptura(fonte);
    EXPECT_EQ(res, 0);
    EXPECT_TRUE(err.empty()) << "Saida de erro inesperada: " << err;
}

} // namespace
