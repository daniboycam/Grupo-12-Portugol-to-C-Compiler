// Testes ponta a ponta da CLI do compilador Portugol (issue #67).
// Executam portugolc, compilam o C++17 gerado com -Werror e conferem a saida.
#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#ifndef _WIN32
#include <sys/wait.h>
#endif

namespace {
namespace fs = std::filesystem;

std::string aspas(const fs::path& caminho) {
    return "\"" + caminho.string() + "\"";
}

// std::system devolve o status codificado no Linux; no Windows, o exit code.
int codigoSaida(int status) {
#ifdef _WIN32
    return status;
#else
    if (status == -1 || !WIFEXITED(status)) return -1;
    return WEXITSTATUS(status);
#endif
}

int comando(const std::string& texto) {
    return codigoSaida(std::system(texto.c_str()));
}

bool temGpp() {
#ifdef _WIN32
    static const bool disponivel = comando("g++ --version > NUL 2>&1") == 0;
#else
    static const bool disponivel = comando("g++ --version > /dev/null 2>&1") == 0;
#endif
    return disponivel;
}

#define EXIGIR_GPP() \
    if (!temGpp()) GTEST_SKIP() << "g++ nao disponivel para testes E2E"

fs::path pastaTeste() {
    const std::string nome =
        ::testing::UnitTest::GetInstance()->current_test_info()->name();
    const fs::path pasta = fs::temp_directory_path() / ("portugolc_issue67_" + nome);
    // Cada teste possui pasta independente. Limpar permite repetir o CTest.
    fs::remove_all(pasta);
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

int executarCompilador(const fs::path& fonte, const fs::path& destino,
                       const fs::path& log, bool saidaPadrao = false) {
    const std::string alvo = saidaPadrao ? "-" : destino.string();
    const std::string redirecionamento = saidaPadrao
        ? " > " + aspas(destino) + " 2> " + aspas(log)
        : " > " + aspas(log) + " 2>&1";
    return comando(aspas(fs::path(PORTUGOLC_BIN)) + " -o \"" + alvo + "\" " +
                   aspas(fonte) + redirecionamento);
}

struct Resultado {
    int geracao = -1;
    int compilacao = -1;
    int execucao = -1;
    std::string logGeracao;
    std::string logCompilacao;
    std::string logExecucao;
    std::string saida;
};

// Fluxo completo: Portugol -> CLI -> .cpp -> g++ -Werror -> executavel.
Resultado compilarExecutar(const std::string& codigoPortugol,
                          const std::string& entradaPadrao = "",
                          bool gerarPelaSaidaPadrao = false) {
    const fs::path pasta = pastaTeste();
    const fs::path fonte = pasta / "programa.por";
    const fs::path cpp = pasta / "programa.cpp";
    const fs::path entrada = pasta / "entrada.txt";
    const fs::path saida = pasta / "saida.txt";
    const fs::path logGeracao = pasta / "geracao.log";
    const fs::path logCompilacao = pasta / "compilacao.log";
    const fs::path logExecucao = pasta / "execucao.log";
#ifdef _WIN32
    const fs::path executavel = pasta / "programa.exe";
#else
    const fs::path executavel = pasta / "programa";
#endif

    Resultado r;
    escrever(fonte, codigoPortugol);
    escrever(entrada, entradaPadrao);
    r.geracao = executarCompilador(fonte, cpp, logGeracao, gerarPelaSaidaPadrao);
    r.logGeracao = ler(logGeracao);
    if (r.geracao != 0 || !fs::exists(cpp)) return r;

    const std::string compilar =
        "g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror " + aspas(cpp) +
        " -o " + aspas(executavel) + " > " + aspas(logCompilacao) + " 2>&1";
    r.compilacao = comando(compilar);
    r.logCompilacao = ler(logCompilacao);
    if (r.compilacao != 0) return r;

    const std::string executar = aspas(executavel) + " < " + aspas(entrada) +
        " > " + aspas(saida) + " 2> " + aspas(logExecucao);
    r.execucao = comando(executar);
    r.logExecucao = ler(logExecucao);
    r.saida = ler(saida);
    return r;
}

void conferirExecucao(const Resultado& r, const std::string& esperado) {
    EXPECT_EQ(r.geracao, 0) << "Geracao falhou:\n" << r.logGeracao;
    EXPECT_EQ(r.compilacao, 0) << "g++ falhou:\n" << r.logCompilacao;
    EXPECT_EQ(r.execucao, 0) << "Programa terminou com erro:\n" << r.logExecucao;
    if (r.geracao == 0 && r.compilacao == 0 && r.execucao == 0) {
        EXPECT_EQ(r.saida, esperado);
    }
}

void conferirErroSemantico(const std::string& fontePortugol,
                          const std::string& trechoMensagem) {
    const fs::path pasta = pastaTeste();
    const fs::path fonte = pasta / "invalido.por";
    const fs::path cpp = pasta / "invalido.cpp";
    const fs::path log = pasta / "erro.log";
    escrever(fonte, fontePortugol);
    const int rc = executarCompilador(fonte, cpp, log);
    EXPECT_NE(rc, 0) << "Fonte invalida foi aceita";
    EXPECT_NE(ler(log).find(trechoMensagem), std::string::npos) << ler(log);
    EXPECT_FALSE(fs::exists(cpp));
}

// ---- Programas validos: geracao, compilacao e execucao ----

TEST(E2ECLI, GeraCompilaEExecutaCpp) {
    EXIGIR_GPP();
    conferirExecucao(compilarExecutar(
        "programa {\n"
        " inteiro valor = 7;\n"
        " escreva(\"resultado=\", valor * 3, \"\\n\");\n"
        "}\n"), "resultado=21\n");
}

TEST(E2ECLI, FuncoesERecursao) {
    EXIGIR_GPP();
    conferirExecucao(compilarExecutar(
        "programa {\n"
        " funcao vazio principal() { escreva(fatorial(5), \"\\n\"); }\n"
        " funcao inteiro fatorial(inteiro n) {\n"
        "  se (n <= 1) { retorne 1; }\n"
        "  retorne n * fatorial(n - 1);\n"
        " }\n"
        "}\n"), "120\n");
}

TEST(E2ECLI, LeiaDaEntradaPadrao) {
    EXIGIR_GPP();
    conferirExecucao(compilarExecutar(
        "programa {\n"
        " inteiro numero;\n"
        " leia(numero);\n"
        " escreva(\"dobro=\", numero * 2, \"\\n\");\n"
        "}\n", "21\n"), "dobro=42\n");
}

TEST(E2ECLI, LacosPareContinueEVetores) {
    EXIGIR_GPP();
    conferirExecucao(compilarExecutar(
        "programa {\n"
        " inteiro v[5];\n"
        " para (inteiro j = 0; j < 5; j += 1) { v[j] = j * j; }\n"
        " inteiro i = 0;\n"
        " inteiro soma = 0;\n"
        " enquanto (i < 5) {\n"
        "  se (i == 1) { i += 1; continue; }\n"
        "  se (i == 4) { pare; }\n"
        "  soma += v[i];\n"
        "  i += 1;\n"
        " }\n"
        " escreva(\"soma=\", soma, \"\\n\");\n"
        "}\n"), "soma=13\n");
}

TEST(E2ECLI, GeraCppNaSaidaPadrao) {
    EXIGIR_GPP();
    conferirExecucao(compilarExecutar(
        "programa {\n escreva(\"stdout-ok\\n\");\n}\n",
        "", true), "stdout-ok\n");
}

TEST(E2ECLI, CaracterConvertidoParaCadeia) {
    EXIGIR_GPP();
    conferirExecucao(compilarExecutar(
        "programa {\n"
        " cadeia s = 'a';\n"
        " escreva(s, \"!\\n\");\n"
        "}\n"), "a!\n");
}

// O programa de demonstracao do README precisa continuar funcionando.
TEST(E2ECLI, ExemploDeDemonstracao) {
    EXIGIR_GPP();
    const std::string fonte = ler(fs::path(DIRETORIO_RAIZ) / "examples" / "demonstracao.por");
    ASSERT_FALSE(fonte.empty());
    conferirExecucao(compilarExecutar(fonte, "Ana 6\n"),
        "Ola, Ana!\n"
        "fatorial(6) = 720 em 6 chamadas\n"
        "primos: 2 3 5 7 11 13 17 19 23 29\n"
        "soma dos pares ate 20 = 110\n"
        "7/2=3 7.0/2=3.5 7%3=1\n"
        "bits: 2 7 5 32 16 -1\n"
        "logico: verdadeiro verdadeiro verdadeiro\n"
        "area(2) = 12.5664\n"
        "sombreado: xyz\n"
        "externo: xy\n"
        "x interno = 11\n"
        "x externo = 1\n");
}

// ---- Rejeicoes da CLI e da analise semantica ----

TEST(E2ECLI, RejeitaSobrescreverArquivoDeEntrada) {
    const fs::path pasta = pastaTeste();
    const fs::path fonte = pasta / "mesmo_arquivo.por";
    const fs::path log = pasta / "erro.log";
    const std::string original = "programa { escreva(\"original\\n\"); }\n";
    escrever(fonte, original);
    const int rc = executarCompilador(fonte, fonte, log);
    EXPECT_EQ(rc, 2) << ler(log);
    EXPECT_NE(ler(log).find("proprio arquivo de entrada"), std::string::npos)
        << ler(log);
    EXPECT_EQ(ler(fonte), original) << "O arquivo de entrada foi alterado";
}

TEST(E2ECLI, RejeitaTipoIncompativel) {
    conferirErroSemantico(
        "programa {\n inteiro x = \"texto\";\n}\n",
        "Falha na analise semantica");
}

// A redeclaracao e acusada pela tabela de simbolos ainda no parser, mas e um
// erro semantico e deve ser relatado como tal.
TEST(E2ECLI, RedeclaracaoFalhaNaAnaliseSemantica) {
    conferirErroSemantico(
        "programa {\n inteiro x;\n inteiro x;\n}\n",
        "Falha na analise semantica (1 erro(s))");
}

// O caractere invalido e ignorado para o parse continuar, mas o programa nao
// pode virar C++.
TEST(E2ECLI, ErroLexicoRecusaOPrograma) {
    conferirErroSemantico(
        "programa {\n inteiro x = 1; @\n escreva(x);\n}\n",
        "Falha na analise lexica (1 erro(s))");
}

TEST(E2ECLI, ErroDeSintaxeContinuaSintatico) {
    conferirErroSemantico(
        "programa {\n inteiro x;\n inteiro x;\n funcao vazio f() { }\n}\n",
        "Falha na analise sintatica");
}

TEST(E2ECLI, RejeitaVariavelNaoDeclarada) {
    conferirErroSemantico(
        "programa {\n escreva(variavelInexistente);\n}\n",
        "nao declarada");
}

TEST(E2ECLI, RejeitaAridadeIncorreta) {
    conferirErroSemantico(
        "programa {\n"
        " funcao vazio principal() { escreva(soma(1)); }\n"
        " funcao inteiro soma(inteiro a, inteiro b) { retorne a + b; }\n"
        "}\n",
        "espera 2 argumento");
}

TEST(E2ECLI, RejeitaPareForaDeLaco) {
    conferirErroSemantico(
        "programa {\n pare;\n}\n",
        "so pode ser usado dentro de um laco");
}

TEST(E2ECLI, RejeitaRetornoIncompativel) {
    conferirErroSemantico(
        "programa {\n"
        " funcao vazio principal() { }\n"
        " funcao inteiro f() { retorne \"texto\"; }\n"
        "}\n",
        "retorno da funcao");
}

}  // namespace
