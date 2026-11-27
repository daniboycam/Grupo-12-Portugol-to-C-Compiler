// Testes do gerador de codigo C++17 (issue #66).
//
// Parte dos testes confere trechos do codigo gerado; a outra compila a saida
// com g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror e executa o binario,
// comparando o que ele escreve. Esses ultimos sao pulados se nao houver g++.
#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "portugol/ast.hpp"
#include "portugol/codegen.hpp"
#include "portugol/semantico.hpp"

extern int yyparse();
extern void yyrestart(FILE* arquivo);
extern FILE* yyin;
extern int yylineno;
extern int yycolumn;

namespace {

// Fonte Portugol -> codigo C++. Falha o teste se houver erro de analise.
std::string gerar(const char* fonte) {
    FILE* arquivo = std::tmpfile();
    if (arquivo == nullptr) throw std::runtime_error("tmpfile falhou");
    const size_t tamanho = std::strlen(fonte);
    if (std::fwrite(fonte, 1, tamanho, arquivo) != tamanho) {
        std::fclose(arquivo);
        throw std::runtime_error("fwrite falhou");
    }
    std::rewind(arquivo);
    yyin = arquivo;
    yyrestart(arquivo);
    yylineno = 1;
    yycolumn = 1;

    testing::internal::CaptureStderr();
    const int rc = yyparse();
    std::ostringstream errosSemanticos;
    const int erros = rc == 0 ? portugol::analisarSemantica(raizAST, errosSemanticos) : -1;
    const std::string erroSintatico = testing::internal::GetCapturedStderr();
    std::fclose(arquivo);
    yyin = nullptr;

    if (rc != 0 || erros != 0) {
        ADD_FAILURE() << "programa invalido:\n" << erroSintatico << errosSemanticos.str();
        return "";
    }
    return portugol::gerarCodigoCpp(raizAST);
}

void contem(const std::string& codigo, const std::string& trecho) {
    EXPECT_NE(codigo.find(trecho), std::string::npos)
        << "trecho esperado: " << trecho << "\ncodigo gerado:\n" << codigo;
}

bool temCompilador() {
    static const int rc = std::system("g++ --version > /dev/null 2>&1");
    return rc == 0;
}

std::string lerArquivo(const std::string& caminho) {
    std::ifstream in(caminho);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// Compila o codigo gerado sem tolerar warnings, executa com a entrada dada e
// devolve o que o programa escreveu.
std::string compilarEExecutar(const std::string& codigo, const std::string& entrada = "") {
    const std::string base = testing::TempDir() + "portugolc_" +
                             testing::UnitTest::GetInstance()->current_test_info()->name();
    { std::ofstream(base + ".cpp") << codigo; }
    { std::ofstream(base + ".in") << entrada; }

    const std::string compilar = "g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror '" + base +
                                 ".cpp' -o '" + base + ".bin' > '" + base + ".log' 2>&1";
    if (std::system(compilar.c_str()) != 0) {
        ADD_FAILURE() << "g++ rejeitou o codigo gerado:\n" << lerArquivo(base + ".log")
                      << "\ncodigo:\n" << codigo;
        return "";
    }
    const std::string executar =
        "'" + base + ".bin' < '" + base + ".in' > '" + base + ".out' 2>&1";
    if (std::system(executar.c_str()) != 0) {
        ADD_FAILURE() << "o programa gerado terminou com erro";
    }
    return lerArquivo(base + ".out");
}

#define EXIGIR_COMPILADOR() \
    if (!temCompilador()) GTEST_SKIP() << "g++ nao disponivel"

// ---- traducao (texto gerado) --------------------------------------------

TEST(CodegenTraducao, TiposEValoresPadrao) {
    const std::string c = gerar(
        "programa {\n inteiro i;\n real r;\n cadeia s;\n caracter ch;\n logico b;\n"
        " inteiro v[4];\n escreva(i, r, s, ch, b, v[0]);\n}\n");
    contem(c, "int main() {");
    contem(c, "int p_i = 0;");
    contem(c, "double p_r = 0.0;");
    contem(c, "std::string p_s = \"\";");
    contem(c, "char p_ch = '\\0';");
    contem(c, "bool p_b = false;");
    contem(c, "int p_v[4] = {};");
    contem(c, "return 0;");
}

TEST(CodegenTraducao, PrincipalViraMainEFuncoesTemPrototipo) {
    const std::string c = gerar(
        "programa {\n"
        " funcao vazio principal() { escreva(dobro(2)); }\n"
        " funcao inteiro dobro(inteiro x) { retorne x * 2; }\n"
        "}\n");
    contem(c, "int p_dobro(int p_x);\n");
    contem(c, "int main() {");
    contem(c, "int p_dobro(int p_x) {\n    return p_x * 2;\n}");
    EXPECT_EQ(c.find("principal"), std::string::npos);
}

TEST(CodegenTraducao, EntradaESaida) {
    const std::string c = gerar("programa {\n inteiro n;\n leia(n);\n escreva(\"n=\", n, \"\\n\");\n}\n");
    contem(c, "std::cin >> p_n;");
    contem(c, "std::cout << \"n=\" << p_n << \"\\n\";");
}

TEST(CodegenTraducao, LogicoEscritoPorExtenso) {
    const std::string c = gerar("programa {\n logico ok = verdadeiro;\n escreva(ok);\n}\n");
    contem(c, "(p_ok ? \"verdadeiro\" : \"falso\")");
}

TEST(CodegenTraducao, ControleDeFluxo) {
    const std::string c = gerar(
        "programa {\n inteiro i = 0;\n"
        " se (i > 0) { i = 1; } senao se (i < 0) { i = 2; } senao { i = 3; }\n"
        " enquanto (i < 10) { i += 1; se (i == 5) { pare; } }\n"
        " para (inteiro j = 0; j < 3; j += 1) { continue; }\n"
        " para (; i < 20; ) { i += 2; }\n"
        "}\n");
    contem(c, "if (p_i > 0) {");
    contem(c, "} else if (p_i < 0) {");
    contem(c, "} else {");
    contem(c, "while (p_i < 10) {");
    contem(c, "break;");
    contem(c, "for (int p_j = 0; p_j < 3; p_j += 1) {");
    contem(c, "continue;");
    contem(c, "for (; p_i < 20; ) {");
}

TEST(CodegenTraducao, ExpressoesComParentesesExplicitos) {
    const std::string c = gerar(
        "programa {\n inteiro a = 1;\n logico ok = a > 0 && a < 5 || !(a == 3);\n"
        " a = (a + 2) * 3 - -a;\n}\n");
    contem(c, "bool p_ok = ((p_a > 0) && (p_a < 5)) || (!(p_a == 3));");
    contem(c, "p_a = ((p_a + 2) * 3) - (-p_a);");
}

TEST(CodegenTraducao, ConcatenacaoComLiteralViraStdString) {
    const std::string c = gerar("programa {\n cadeia s = \"a\" + \"b\";\n}\n");
    contem(c, "std::string(\"a\") + std::string(\"b\")");
}

TEST(CodegenTraducao, LiteralRealMantemTipoReal) {
    const std::string c = gerar("programa {\n real r = 4.0 / 2;\n escreva(r);\n}\n");
    contem(c, "double p_r = 4.0 / 2;");
}

TEST(CodegenTraducao, EscapaLiterais) {
    const std::string c = gerar(
        "programa {\n cadeia s = \"aspas \\\" barra \\\\ tab \\t\";\n caracter c = '\\'';\n"
        " escreva(s, c);\n}\n");
    contem(c, "\"aspas \\\" barra \\\\ tab \\t\"");
    contem(c, "'\\''");
}

TEST(CodegenTraducao, IdentificadoresGanhamPrefixo) {
    // Palavras reservadas do C++ e nomes que so diferem por "_" nao colidem.
    const std::string c = gerar(
        "programa {\n inteiro int = 1;\n inteiro int_ = 2;\n inteiro main = int + int_;\n"
        " escreva(main);\n}\n");
    contem(c, "int p_int = 1;");
    contem(c, "int p_int_ = 2;");
    contem(c, "int p_main = p_int + p_int_;");
}

TEST(CodegenTraducao, VariavelQueEscondeOutraGanhaNomeProprio) {
    const std::string c = gerar(
        "programa {\n"
        " funcao inteiro f(inteiro x) {\n"
        "  se (x > 0) { inteiro x = x + 1; retorne x; }\n"
        "  retorne x;\n"
        " }\n"
        " funcao vazio principal() { escreva(f(10)); }\n"
        "}\n");
    contem(c, "int p2_x = p_x + 1;");
    contem(c, "return p2_x;");
    contem(c, "    return p_x;\n");
}

TEST(CodegenTraducao, CaracterParaCadeiaViraStdString) {
    const std::string c = gerar(
        "programa {\n"
        " funcao cadeia f(cadeia s) { retorne 'z'; }\n"
        " funcao vazio principal() { cadeia s = 'a'; s = 'b'; escreva(f('c'), s); }\n"
        "}\n");
    contem(c, "std::string p_s = std::string(1, 'a');");
    contem(c, "p_s = std::string(1, 'b');");
    contem(c, "return std::string(1, 'z');");
    contem(c, "p_f(std::string(1, 'c'))");
}

TEST(CodegenTraducao, VariavelNuncaLidaRecebeMaybeUnused) {
    const std::string c = gerar(
        "programa {\n funcao vazio principal() { inteiro x; x = 1; escreva(f(2)); }\n"
        " funcao inteiro f(inteiro a) { retorne 0; }\n}\n");
    contem(c, "[[maybe_unused]] int p_x = 0;");
    contem(c, "int p_f([[maybe_unused]] int p_a) {");
}

TEST(CodegenTraducao, RetorneSemValorNaPrincipalViraReturnZero) {
    const std::string c = gerar(
        "programa {\n funcao vazio principal() { inteiro x = 1; se (x > 0) { retorne; } }\n}\n");
    contem(c, "        return 0;\n");
}

// ---- compilacao e execucao ----------------------------------------------

TEST(CodegenExecucao, ProgramaSemFuncoes) {
    EXIGIR_COMPILADOR();
    const std::string saida = compilarEExecutar(gerar(
        "programa {\n"
        " inteiro v[5];\n inteiro soma = 0;\n"
        " para (inteiro i = 0; i < 5; i += 1) { v[i] = i * i; soma += v[i]; }\n"
        " escreva(\"soma=\", soma, \" ultimo=\", v[4], \"\\n\");\n"
        "}\n"));
    EXPECT_EQ(saida, "soma=30 ultimo=16\n");
}

TEST(CodegenExecucao, FuncoesRecursaoEGlobais) {
    EXIGIR_COMPILADOR();
    const std::string saida = compilarEExecutar(gerar(
        "programa {\n"
        " inteiro chamadas = 0;\n"
        " funcao vazio principal() {\n"
        "  escreva(fib(10), \" \", chamadas > 0, \" \", media(3, 4), \"\\n\");\n"
        " }\n"
        " funcao inteiro fib(inteiro n) {\n"
        "  chamadas += 1;\n"
        "  se (n < 2) { retorne n; }\n"
        "  retorne fib(n - 1) + fib(n - 2);\n"
        " }\n"
        " funcao real media(real a, real b) { retorne (a + b) / 2; }\n"
        "}\n"));
    EXPECT_EQ(saida, "55 verdadeiro 3.5\n");
}

TEST(CodegenExecucao, LeituraEDesvios) {
    EXIGIR_COMPILADOR();
    const std::string saida = compilarEExecutar(gerar(
        "programa {\n"
        " inteiro n;\n inteiro pares = 0;\n cadeia nome;\n"
        " leia(nome);\n leia(n);\n"
        " enquanto (verdadeiro) {\n"
        "  n -= 1;\n"
        "  se (n < 0) { pare; }\n"
        "  se (n % 2 != 0) { continue; }\n"
        "  pares += 1;\n"
        " }\n"
        " escreva(\"Ola, \" + nome + '!', \" pares=\", pares, \"\\n\");\n"
        "}\n"),
        "Ana 7\n");
    EXPECT_EQ(saida, "Ola, Ana! pares=4\n");
}

TEST(CodegenExecucao, SemanticaDeDivisaoEOperadores) {
    EXIGIR_COMPILADOR();
    const std::string saida = compilarEExecutar(gerar(
        "programa {\n"
        " escreva(7 / 2, \" \", 7.0 / 2, \" \", 7 % 3, \" \", (6 & 3) | 8, \" \", 1 << 4,"
        " \" \", 'a' < 'b', \" \", \"abc\" == \"abc\", \"\\n\");\n"
        "}\n"));
    EXPECT_EQ(saida, "3 3.5 1 10 16 verdadeiro verdadeiro\n");
}

TEST(CodegenExecucao, CaracterParaCadeia) {
    EXIGIR_COMPILADOR();
    const std::string saida = compilarEExecutar(gerar(
        "programa {\n"
        " funcao cadeia repete(cadeia s, inteiro n) {\n"
        "  cadeia r = \"\";\n"
        "  para (inteiro i = 0; i < n; i += 1) { r += s; }\n"
        "  retorne r;\n"
        " }\n"
        " funcao cadeia inicial(cadeia s) { retorne 'I'; }\n"
        " funcao vazio principal() {\n"
        "  caracter c = 'x';\n"
        "  cadeia s = c;\n"
        "  escreva(s, \" \", repete(c, 3), \" \", inicial(\"abc\"), \"\\n\");\n"
        " }\n"
        "}\n"));
    EXPECT_EQ(saida, "x xxx I\n");
}

TEST(CodegenExecucao, VariavelInternaUsaAExternaNoInicializador) {
    EXIGIR_COMPILADOR();
    const std::string saida = compilarEExecutar(gerar(
        "programa {\n"
        " funcao inteiro f(inteiro x) {\n"
        "  se (x > 0) { inteiro x = x + 1; retorne x; }\n"
        "  retorne 0;\n"
        " }\n"
        " funcao inteiro dobro(inteiro n) { retorne n * 2; }\n"
        " funcao vazio principal() {\n"
        "  inteiro dobro = dobro(2);\n"
        "  escreva(f(10), \" \", dobro, \"\\n\");\n"
        " }\n"
        "}\n"));
    EXPECT_EQ(saida, "11 4\n");
}

// Desde a #64 a tabela do parser aceita esconder nomes em blocos internos;
// cada nivel precisa virar uma variavel propria no C++.
TEST(CodegenExecucao, SombreamentoEmBlocosInternos) {
    EXIGIR_COMPILADOR();
    const std::string saida = compilarEExecutar(gerar(
        "programa {\n"
        " inteiro x = 1;\n"
        " cadeia s = \"fora\";\n"
        " se (x == 1) {\n"
        "  inteiro x = 2;\n"
        "  se (x == 2) { inteiro x = x + 1; escreva(x); }\n"
        "  escreva(x);\n"
        " } senao { inteiro x = 9; escreva(x); }\n"
        " escreva(x, \" \");\n"
        " para (inteiro x = x * 7; x < 9; x += 1) { escreva(x); }\n"
        " enquanto (verdadeiro) { cadeia s = s + '!'; escreva(\" \", s); pare; }\n"
        " escreva(\" \", x, s, \"\\n\");\n"
        "}\n"));
    EXPECT_EQ(saida, "321 78 fora! 1fora\n");
}

TEST(CodegenExecucao, NomesDaBibliotecaPadraoEMacros) {
    EXIGIR_COMPILADOR();
    const std::string saida = compilarEExecutar(gerar(
        "programa {\n"
        " inteiro time = 3;\n"
        " funcao inteiro div(inteiro a, inteiro b) { retorne a / b; }\n"
        " funcao inteiro rand() { retorne 4; }\n"
        " funcao vazio principal() {\n"
        "  inteiro EOF = 1;\n inteiro NULL = 2;\n inteiro int = 5;\n inteiro int_ = 6;\n"
        "  escreva(div(9, time), rand(), EOF, NULL, int, int_, \"\\n\");\n"
        " }\n"
        "}\n"));
    EXPECT_EQ(saida, "341256\n");
}

}  // namespace
