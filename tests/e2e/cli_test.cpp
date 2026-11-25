
#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace {

namespace fs = std::filesystem;

fs::path pastaTeste() {
    const std::string nome =
        ::testing::UnitTest::GetInstance()
            ->current_test_info()->name();

    const fs::path pasta =
        fs::temp_directory_path() / ("portugolc_issue67_" + nome);

    fs::create_directories(pasta);
    return pasta;
}

void escrever(const fs::path& caminho, const std::string& texto) {
    std::ofstream arquivo(caminho);
    arquivo << texto;
}

std::string ler(const fs::path& caminho) {
    std::ifstream arquivo(caminho);
    std::ostringstream conteudo;
    conteudo << arquivo.rdbuf();
    return conteudo.str();
}

int executarCompilador(
    const fs::path& entrada,
    const fs::path& saida,
    const fs::path& log
) {
    const std::string comando =
        "\"" + std::string(PORTUGOLC_BIN) +
        "\" -o \"" + saida.string() +
        "\" \"" + entrada.string() +
        "\" > \"" + log.string() + "\" 2>&1";

    return std::system(comando.c_str());
}

TEST(E2ECLI, GeraCompilaEExecutaCpp) {
    const fs::path pasta = pastaTeste();
    const fs::path fonte = pasta / "valido.por";
    const fs::path cpp = pasta / "gerado.cpp";
    const fs::path log = pasta / "compilador.log";
    const fs::path saida = pasta / "saida.txt";

#ifdef _WIN32
    const fs::path executavel = pasta / "gerado.exe";
#else
    const fs::path executavel = pasta / "gerado";
#endif

    escrever(fonte,
        "programa {\n"
        " inteiro valor = 7;\n"
        " escreva(\"resultado=\", valor * 3, \"\\n\");\n"
        "}\n");

    fs::remove(cpp);

    ASSERT_EQ(executarCompilador(fonte, cpp, log), 0)
        << ler(log);

    ASSERT_TRUE(fs::exists(cpp));

    const std::string compilar =
        "g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror \"" +
        cpp.string() + "\" -o \"" + executavel.string() +
        "\" > \"" + log.string() + "\" 2>&1";

    ASSERT_EQ(std::system(compilar.c_str()), 0)
        << ler(log);

    const std::string executar =
        "\"" + executavel.string() +
        "\" > \"" + saida.string() + "\" 2>&1";

    ASSERT_EQ(std::system(executar.c_str()), 0);
    EXPECT_EQ(ler(saida), "resultado=21\n");
}

TEST(E2ECLI, RejeitaTipoIncompativel) {
    const fs::path pasta = pastaTeste();
    const fs::path fonte = pasta / "tipo_invalido.por";
    const fs::path cpp = pasta / "tipo_invalido.cpp";
    const fs::path log = pasta / "erro.log";

    escrever(fonte,
        "programa {\n"
        " inteiro x = \"texto\";\n"
        "}\n");

    fs::remove(cpp);

    EXPECT_NE(executarCompilador(fonte, cpp, log), 0);
    EXPECT_NE(
        ler(log).find("Falha na analise semantica"),
        std::string::npos
    );
    EXPECT_FALSE(fs::exists(cpp));
}

TEST(E2ECLI, RejeitaVariavelNaoDeclarada) {
    const fs::path pasta = pastaTeste();
    const fs::path fonte = pasta / "variavel_invalida.por";
    const fs::path cpp = pasta / "variavel_invalida.cpp";
    const fs::path log = pasta / "erro.log";

    escrever(fonte,
        "programa {\n"
        " escreva(variavelInexistente);\n"
        "}\n");

    fs::remove(cpp);

    EXPECT_NE(executarCompilador(fonte, cpp, log), 0);
    EXPECT_NE(
        ler(log).find("nao declarada"),
        std::string::npos
    );
    EXPECT_FALSE(fs::exists(cpp));
}

} // namespace
