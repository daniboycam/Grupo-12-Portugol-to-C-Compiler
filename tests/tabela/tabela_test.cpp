// Testes da tabela de simbolos com escopos (issue #64).
//
// A primeira parte usa a classe TabelaSimbolos diretamente; a segunda passa
// programas pelo parser, que preenche a tabela global durante a analise.
#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#include "portugol/tabela.hpp"

extern int yyparse();
extern void yyrestart(FILE* arquivo);
extern FILE* yyin;
extern int yylineno;
extern int yycolumn;

namespace {

using portugol::CategoriaSimbolo;
using portugol::Simbolo;
using portugol::TabelaSimbolos;

// ---- classe TabelaSimbolos ---------------------------------------------

TEST(TabelaEscopos, ComecaSoComOEscopoGlobal) {
    TabelaSimbolos t;
    EXPECT_EQ(t.nivel(), 1u);
    EXPECT_EQ(t.tamanho(), 0u);
    EXPECT_TRUE(t.inserirSimbolo("x", "inteiro", 1));
    EXPECT_EQ(t.simbolos().front().escopo, "global");
}

TEST(TabelaEscopos, RecusaNomeRepetidoNoMesmoEscopo) {
    TabelaSimbolos t;
    EXPECT_TRUE(t.inserirSimbolo("x", "inteiro", 1));
    EXPECT_FALSE(t.inserirSimbolo("x", "real", 2));
    EXPECT_EQ(t.tamanho(), 1u);
    EXPECT_EQ(t.buscarSimbolo("x")->tipo, "inteiro");
}

TEST(TabelaEscopos, EscopoInternoEscondeOExterno) {
    TabelaSimbolos t;
    t.inserirSimbolo("x", "inteiro", 1);
    t.entrarEscopo("bloco");
    EXPECT_TRUE(t.inserirSimbolo("x", "real", 2));
    EXPECT_EQ(t.buscarSimbolo("x")->tipo, "real");
    t.sairEscopo();
    EXPECT_EQ(t.buscarSimbolo("x")->tipo, "inteiro");
    // O simbolo do escopo fechado continua guardado para a impressao.
    EXPECT_EQ(t.tamanho(), 2u);
}

TEST(TabelaEscopos, NomeDeEscopoFechadoNaoEEncontrado) {
    TabelaSimbolos t;
    t.entrarEscopo("f");
    t.inserirSimbolo("local", "inteiro", 1);
    t.sairEscopo();
    EXPECT_EQ(t.buscarSimbolo("local"), nullptr);
    EXPECT_TRUE(t.inserirSimbolo("local", "cadeia", 2));
}

TEST(TabelaEscopos, BuscaNoEscopoAtualIgnoraOsDeFora) {
    TabelaSimbolos t;
    t.inserirSimbolo("g", "inteiro", 1);
    t.entrarEscopo("f");
    EXPECT_EQ(t.buscarSimboloEscopoAtual("g"), nullptr);
    EXPECT_NE(t.buscarSimbolo("g"), nullptr);
}

TEST(TabelaEscopos, GlobalNuncaESaido) {
    TabelaSimbolos t;
    t.sairEscopo();
    t.voltarAoNivel(0);
    EXPECT_EQ(t.nivel(), 1u);
    EXPECT_TRUE(t.inserirSimbolo("x", "inteiro", 1));
}

TEST(TabelaEscopos, VoltarAoNivelFechaVariosEscopos) {
    TabelaSimbolos t;
    const std::size_t antes = t.nivel();
    t.entrarEscopo("f");
    t.entrarEscopo("bloco");
    t.entrarEscopo("para");
    t.voltarAoNivel(antes);
    EXPECT_EQ(t.nivel(), antes);
}

TEST(TabelaEscopos, EscopoGuardaOCaminho) {
    TabelaSimbolos t;
    t.entrarEscopo("principal");
    t.entrarEscopo("para");
    t.inserirSimbolo("i", "inteiro", 3);
    EXPECT_EQ(t.simbolos().back().escopo, "principal/para");
}

TEST(TabelaEscopos, LimparVoltaAoEstadoInicial) {
    TabelaSimbolos t;
    t.entrarEscopo("f");
    t.inserirSimbolo("x", "inteiro", 1);
    t.limpar();
    EXPECT_EQ(t.nivel(), 1u);
    EXPECT_EQ(t.tamanho(), 0u);
    EXPECT_EQ(t.buscarSimbolo("x"), nullptr);
}

TEST(TabelaEscopos, PonteirosContinuamValidosAposMuitasInsercoes) {
    TabelaSimbolos t;
    t.inserirSimbolo("primeiro", "inteiro", 1);
    const Simbolo* primeiro = t.buscarSimbolo("primeiro");
    for (int i = 0; i < 1000; ++i) t.inserirSimbolo("v" + std::to_string(i), "inteiro", i);
    EXPECT_EQ(t.buscarSimbolo("primeiro"), primeiro);
    EXPECT_EQ(primeiro->nome, "primeiro");
}

// ---- tabela preenchida pelo parser --------------------------------------

struct Resultado {
    int codigo;
    std::string mensagens;
};

Resultado analisar(const char* fonte) {
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
    const int codigo = yyparse();
    const std::string mensagens = testing::internal::GetCapturedStderr();
    std::fclose(arquivo);
    yyin = nullptr;
    return {codigo, mensagens};
}

void esperarValido(const char* fonte) {
    const Resultado r = analisar(fonte);
    EXPECT_EQ(r.codigo, 0) << r.mensagens;
    EXPECT_EQ(r.mensagens, "");
}

void esperarRedeclaracao(const char* fonte, int linha, const std::string& nome) {
    const Resultado r = analisar(fonte);
    EXPECT_NE(r.codigo, 0);
    const std::string esperado = "Erro semântico na linha " + std::to_string(linha) +
                                 ": identificador '" + nome + "' ja declarado no mesmo escopo";
    EXPECT_NE(r.mensagens.find(esperado), std::string::npos)
        << "esperado: " << esperado << "\nobtido:\n" << r.mensagens;
}

// Simbolo da ultima analise com o nome e o escopo dados.
const Simbolo* simbolo(const std::string& nome, const std::string& escopo) {
    for (const Simbolo& s : portugol::tabela.simbolos()) {
        if (s.nome == nome && s.escopo == escopo) return &s;
    }
    return nullptr;
}

TEST(TabelaParser, MesmoLocalEmFuncoesDiferentes) {
    esperarValido(
        "programa {\n"
        " funcao inteiro dobro(inteiro n) { inteiro r = n * 2; retorne r; }\n"
        " funcao vazio principal() { inteiro r = dobro(3); escreva(r); }\n"
        "}\n");
    EXPECT_NE(simbolo("r", "dobro"), nullptr);
    EXPECT_NE(simbolo("r", "principal"), nullptr);
}

TEST(TabelaParser, MesmoParametroEmFuncoesDiferentes) {
    esperarValido(
        "programa {\n"
        " funcao inteiro a(inteiro n) { retorne n; }\n"
        " funcao inteiro b(inteiro n) { retorne n; }\n"
        " funcao vazio principal() { escreva(a(1) + b(2)); }\n"
        "}\n");
}

TEST(TabelaParser, DoisParaComAMesmaVariavel) {
    esperarValido(
        "programa {\n funcao vazio principal() {\n"
        "  para (inteiro i = 0; i < 2; i += 1) { }\n"
        "  para (inteiro i = 0; i < 3; i += 1) { }\n"
        " }\n}\n");
    EXPECT_NE(simbolo("i", "principal/para"), nullptr);
}

TEST(TabelaParser, SombreamentoEmBlocoInterno) {
    esperarValido(
        "programa {\n inteiro x = 1;\n funcao vazio principal() {\n"
        "  se (x > 0) { inteiro x = 2; escreva(x); }\n"
        "  senao { inteiro x = 3; escreva(x); }\n"
        "  enquanto (x < 0) { inteiro x = 4; escreva(x); }\n"
        " }\n}\n");
    EXPECT_NE(simbolo("x", "global"), nullptr);
    EXPECT_NE(simbolo("x", "principal/bloco"), nullptr);
}

TEST(TabelaParser, ParametrosEntramNoEscopoDaFuncao) {
    esperarValido(
        "programa {\n funcao real media(real a, real b) { retorne (a + b) / 2; }\n"
        " funcao vazio principal() { escreva(media(1.0, 2.0)); }\n}\n");
    const Simbolo* a = simbolo("a", "media");
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(a->categoria, CategoriaSimbolo::PARAMETRO);
    EXPECT_EQ(a->tipo, "real");
    EXPECT_EQ(simbolo("a", "global"), nullptr);
}

TEST(TabelaParser, FuncaoGuardaAssinatura) {
    esperarValido(
        "programa {\n funcao cadeia rotulo(inteiro n, caracter c) { retorne \"x\"; }\n"
        " funcao vazio principal() { escreva(rotulo(1, 'a')); }\n}\n");
    const Simbolo* f = simbolo("rotulo", "global");
    ASSERT_NE(f, nullptr);
    EXPECT_EQ(f->categoria, CategoriaSimbolo::FUNCAO);
    EXPECT_EQ(f->tipo_retorno, "cadeia");
    EXPECT_EQ(f->tipos_parametros, (std::vector<std::string>{"inteiro", "caracter"}));
    EXPECT_EQ(f->linha, 2);
}

TEST(TabelaParser, VetorGuardaDimensao) {
    esperarValido("programa {\n inteiro v[5], n;\n v[0] = 1; n = v[0];\n}\n");
    EXPECT_EQ(simbolo("v", "global")->dimensao, 5);
    EXPECT_EQ(simbolo("n", "global")->dimensao, 0);
}

TEST(TabelaParser, RedeclaracaoNoMesmoEscopo) {
    esperarRedeclaracao(
        "programa {\n funcao vazio principal() {\n"
        "  inteiro y = 1;\n  inteiro y = 2;\n }\n}\n",
        4, "y");
}

TEST(TabelaParser, RedeclaracaoInformaALinhaAnterior) {
    const Resultado r = analisar("programa {\n inteiro y;\n\n cadeia y;\n}\n");
    EXPECT_NE(r.mensagens.find("ja declarado no mesmo escopo (linha 2)"), std::string::npos)
        << r.mensagens;
}

TEST(TabelaParser, ParametroRepetido) {
    esperarRedeclaracao(
        "programa {\n funcao vazio principal() { }\n"
        " funcao inteiro f(inteiro a, inteiro a) { retorne a; }\n}\n",
        3, "a");
}

TEST(TabelaParser, ParametroRedeclaradoNoCorpo) {
    esperarRedeclaracao(
        "programa {\n funcao vazio principal() { }\n"
        " funcao inteiro f(inteiro a) {\n  inteiro a = 3;\n  retorne a;\n }\n}\n",
        4, "a");
}

TEST(TabelaParser, VariavelDoParaRedeclaradaNoCorpo) {
    esperarRedeclaracao(
        "programa {\n para (inteiro i = 0; i < 2; i += 1) {\n  inteiro i = 5;\n }\n}\n",
        3, "i");
}

TEST(TabelaParser, FuncaoDuplicada) {
    esperarRedeclaracao(
        "programa {\n funcao vazio principal() { }\n funcao vazio f() { }\n"
        " funcao vazio f() { }\n}\n",
        4, "f");
}

TEST(TabelaParser, GlobalEFuncaoComOMesmoNome) {
    esperarRedeclaracao(
        "programa {\n inteiro g = 1;\n funcao vazio g() { }\n"
        " funcao vazio principal() { }\n}\n",
        3, "g");
}

TEST(TabelaParser, ErroDentroDoBlocoNaoAtrapalhaOsEscoposDeFora) {
    // Depois da recuperacao do erro dentro do bloco, o "b" de fora continua
    // no escopo da funcao e a redeclaracao e detectada.
    const Resultado r = analisar(
        "programa {\n funcao vazio principal() {\n"
        "  se (verdadeiro) { inteiro a = ; }\n"
        "  inteiro b = 1;\n  inteiro b = 2;\n }\n}\n");
    EXPECT_NE(r.codigo, 0);
    EXPECT_NE(r.mensagens.find("identificador 'b' ja declarado no mesmo escopo"),
              std::string::npos)
        << r.mensagens;
    EXPECT_NE(simbolo("b", "principal"), nullptr);
}

TEST(TabelaParser, CadaAnaliseComecaComTabelaLimpa) {
    esperarValido("programa {\n funcao vazio principal() { inteiro x = 1; escreva(x); }\n}\n");
    esperarValido("programa {\n inteiro x = 2;\n escreva(x);\n}\n");
    ASSERT_EQ(portugol::tabela.tamanho(), 1u);
    EXPECT_EQ(portugol::tabela.simbolos().front().escopo, "global");
    EXPECT_EQ(portugol::tabela.nivel(), 1u);
}

}  // namespace
