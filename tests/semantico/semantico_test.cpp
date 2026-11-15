// Testes da analise semantica sobre a AST (issue #65).
#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <string>

#include "portugol/ast.hpp"
#include "portugol/semantico.hpp"

extern int yyparse();
extern void yyrestart(FILE* arquivo);
extern FILE* yyin;
extern int yylineno;
extern int yycolumn;

namespace {

struct Resultado {
    int erros;
    std::string mensagens;
};

// Analisa a fonte (sintatica + semantica). Falha o teste se o programa nem
// passar pelo parser, porque o objetivo aqui e exercitar a fase semantica.
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
    const int rc = yyparse();
    const std::string erroSintatico = testing::internal::GetCapturedStderr();
    std::fclose(arquivo);
    yyin = nullptr;
    if (rc != 0) {
        ADD_FAILURE() << "programa nao passou na analise sintatica:\n" << erroSintatico;
        return {-1, erroSintatico};
    }

    std::ostringstream saida;
    const int erros = portugol::analisarSemantica(raizAST, saida);
    return {erros, saida.str()};
}

void esperarValido(const char* fonte) {
    const Resultado r = analisar(fonte);
    EXPECT_EQ(r.erros, 0) << r.mensagens;
    EXPECT_EQ(r.mensagens, "");
}

// Espera ao menos um erro contendo o trecho, na linha informada.
void esperarErro(const char* fonte, int linha, const std::string& trecho) {
    const Resultado r = analisar(fonte);
    EXPECT_GT(r.erros, 0) << "nenhum erro relatado";
    const std::string prefixo = "Erro semântico na linha " + std::to_string(linha) + ": ";
    bool achou = false;
    std::istringstream linhas(r.mensagens);
    for (std::string l; std::getline(linhas, l);) {
        if (l.rfind(prefixo, 0) == 0 && l.find(trecho) != std::string::npos) achou = true;
    }
    EXPECT_TRUE(achou) << "esperado '" << trecho << "' na linha " << linha << "; obtido:\n"
                       << r.mensagens;
}

// ---- programas validos -------------------------------------------------

TEST(SemanticoValido, ProgramaSemFuncoes) {
    esperarValido(
        "programa {\n"
        "    inteiro i = 0;\n"
        "    real media = 0;\n"
        "    cadeia nome = \"ana\";\n"
        "    caracter c = 'x';\n"
        "    logico ok = i < 10 && !falso;\n"
        "    inteiro v[5];\n"
        "    nome = nome + c + \" silva\";\n"
        "    enquanto (i < 5) {\n"
        "        v[i] = i * 2;\n"
        "        media += v[i];\n"
        "        i += 1;\n"
        "    }\n"
        "    para (inteiro j = 0; j < 5; j += 1) {\n"
        "        se (v[j] % 2 == 0) { continue; } senao { pare; }\n"
        "    }\n"
        "    leia(v[0]);\n"
        "    escreva(nome, \" \", media / 5, ok, c);\n"
        "}\n");
}

TEST(SemanticoValido, FuncoesRecursaoEChamadaAntesDaDefinicao) {
    esperarValido(
        "programa {\n"
        "    inteiro contador = 0;\n"
        "    funcao vazio principal() {\n"
        "        inteiro n = 5;\n"
        "        escreva(fatorial(n), \"\\n\");\n"
        "        mostrar(media(1, 2.5));\n"
        "    }\n"
        "    funcao inteiro fatorial(inteiro n) {\n"
        "        contador += 1;\n"
        "        se (n <= 1) { retorne 1; } senao { retorne n * fatorial(n - 1); }\n"
        "    }\n"
        "    funcao real media(real a, real b) { retorne (a + b) / 2; }\n"
        "    funcao vazio mostrar(real x) { escreva(x); retorne; }\n"
        "}\n");
}

TEST(SemanticoValido, EnquantoVerdadeiroComRetorneConta) {
    esperarValido(
        "programa {\n"
        "    funcao vazio principal() { escreva(achar()); }\n"
        "    funcao inteiro achar() {\n"
        "        inteiro k = 0;\n"
        "        enquanto (verdadeiro) { se (k > 3) { retorne k; } k += 1; }\n"
        "    }\n"
        "}\n");
}

TEST(SemanticoValido, MarcaTiposEUsoNaAST) {
    const Resultado r = analisar(
        "programa {\n"
        "    inteiro a = 1;\n"
        "    inteiro b;\n"
        "    b = a + 2;\n"
        "}\n");
    ASSERT_EQ(r.erros, 0) << r.mensagens;
    ASSERT_NE(raizAST, nullptr);
    const NoAST* declA = raizAST->filhos[0]->filhos[0];
    const NoAST* declB = raizAST->filhos[1]->filhos[0];
    EXPECT_TRUE(declA->usado);
    EXPECT_FALSE(declB->usado);  // so recebe valor, nunca e lido
    const NoAST* atrib = raizAST->filhos[2];
    EXPECT_EQ(atrib->dir->tipoDado, "inteiro");
}

// ---- declaracoes e escopos ---------------------------------------------

TEST(SemanticoDeclaracao, VariavelNaoDeclarada) {
    esperarErro("programa {\n inteiro a = 1;\n a = b + 1;\n}\n", 3,
                "variavel 'b' nao declarada");
}

TEST(SemanticoDeclaracao, UsoAntesDaDeclaracao) {
    esperarErro("programa {\n x = 1;\n inteiro x;\n}\n", 2, "variavel 'x' nao declarada");
}

TEST(SemanticoDeclaracao, VariavelForaDoEscopoDoBloco) {
    esperarErro("programa {\n se (verdadeiro) {\n inteiro t = 1;\n }\n escreva(t);\n}\n", 5,
                "variavel 't' nao declarada");
}

TEST(SemanticoDeclaracao, VariavelDoParaNaoVazaDoLaco) {
    esperarErro("programa {\n para (inteiro k = 0; k < 2; k += 1) { }\n escreva(k);\n}\n", 3,
                "variavel 'k' nao declarada");
}

TEST(SemanticoDeclaracao, ParametroRepetido) {
    esperarErro(
        "programa {\n funcao vazio principal() { }\n"
        " funcao inteiro f(inteiro a, inteiro a) { retorne a; }\n}\n",
        3, "parametro 'a' repetido");
}

TEST(SemanticoDeclaracao, FuncaoDuplicada) {
    esperarErro(
        "programa {\n funcao vazio principal() { }\n funcao vazio f() { }\n"
        " funcao vazio f() { }\n}\n",
        4, "'f' ja declarado");
}

TEST(SemanticoDeclaracao, FuncaoDentroDeFuncao) {
    esperarErro(
        "programa {\n funcao vazio principal() {\n"
        "  funcao vazio interna() { }\n }\n}\n",
        3, "declarada dentro de outro bloco");
}

TEST(SemanticoDeclaracao, ComandoForaDeFuncao) {
    esperarErro("programa {\n inteiro g = 0;\n g = 2;\n funcao vazio principal() { }\n}\n", 3,
                "comando fora de funcao");
}

// ---- tipos -------------------------------------------------------------

TEST(SemanticoTipos, InicializacaoIncompativel) {
    esperarErro("programa {\n inteiro x = \"oi\";\n}\n", 2,
                "nao e possivel inicializar 'x' (inteiro) com um valor do tipo cadeia");
}

TEST(SemanticoTipos, RealParaInteiroNaoEPermitido) {
    esperarErro("programa {\n inteiro x;\n x = 2.5;\n}\n", 3,
                "atribuir um valor do tipo real a 'x' (inteiro)");
}

TEST(SemanticoTipos, AtribuicaoCompostaIncompativel) {
    esperarErro("programa {\n logico b = falso;\n b += 1;\n}\n", 3,
                "operador '+=' nao se aplica a 'b' (logico)");
}

TEST(SemanticoTipos, RestoExigeInteiros) {
    esperarErro("programa {\n real r = 1.5;\n inteiro x = 0;\n x = x % r;\n}\n", 4,
                "operador '%' nao se aplica aos tipos inteiro e real");
}

TEST(SemanticoTipos, LogicoExigeOperandosLogicos) {
    esperarErro("programa {\n inteiro a = 1;\n logico ok = a && verdadeiro;\n}\n", 3,
                "operador '&&' nao se aplica aos tipos inteiro e logico");
}

TEST(SemanticoTipos, RelacionalEntreCadeiaEInteiro) {
    esperarErro("programa {\n cadeia s = \"a\";\n logico ok = s < 3;\n}\n", 3,
                "operador '<' nao se aplica aos tipos cadeia e inteiro");
}

TEST(SemanticoTipos, CondicaoDoSeDeveSerLogica) {
    esperarErro("programa {\n inteiro a = 1;\n se (a) { }\n}\n", 3,
                "condicao do 'se' deve ser do tipo logico (recebeu inteiro)");
}

TEST(SemanticoTipos, CondicaoDoEnquantoDeveSerLogica) {
    esperarErro("programa {\n enquanto (\"x\") { }\n}\n", 2,
                "condicao do 'enquanto' deve ser do tipo logico");
}

TEST(SemanticoTipos, NaoUnarioExigeLogico) {
    esperarErro("programa {\n logico b = !5;\n}\n", 2,
                "operador unario '!' nao se aplica ao tipo inteiro");
}

TEST(SemanticoTipos, DivisaoPorZeroLiteral) {
    esperarErro("programa {\n inteiro a = 4;\n a = a / 0;\n}\n", 3, "divisao por zero");
}

TEST(SemanticoTipos, ErroNaoGeraCascata) {
    const Resultado r = analisar("programa {\n inteiro a = (b + 1) * 2 - 3;\n}\n");
    EXPECT_EQ(r.erros, 1) << r.mensagens;
}

// ---- vetores -----------------------------------------------------------

TEST(SemanticoVetor, IndiceDeveSerInteiro) {
    esperarErro("programa {\n inteiro v[3];\n v[1.5] = 2;\n}\n", 3,
                "indice do vetor 'v' deve ser do tipo inteiro (recebeu real)");
}

TEST(SemanticoVetor, IndiceConstanteForaDosLimites) {
    esperarErro("programa {\n inteiro v[3];\n escreva(v[3]);\n}\n", 3,
                "indice 3 fora dos limites do vetor 'v' (tamanho 3)");
}

TEST(SemanticoVetor, VetorSemIndice) {
    esperarErro("programa {\n inteiro v[3];\n inteiro x = v;\n}\n", 3,
                "vetor 'v' deve ser acessado com indice");
}

TEST(SemanticoVetor, EscalarIndexado) {
    esperarErro("programa {\n inteiro x = 1;\n x[0] = 2;\n}\n", 3,
                "'x' nao e um vetor");
}

TEST(SemanticoVetor, TamanhoZero) {
    esperarErro("programa {\n inteiro v[0];\n}\n", 2, "tamanho do vetor 'v' deve ser maior que zero");
}

// ---- chamadas de funcao ------------------------------------------------

constexpr const char* PREFIXO_FUNCOES =
    "programa {\n"                                                  // 1
    " funcao inteiro soma(inteiro a, inteiro b) { retorne a + b; }\n"  // 2
    " funcao vazio nada() { }\n";                                   // 3

std::string comPrincipal(const std::string& corpo) {
    return std::string(PREFIXO_FUNCOES) + " funcao vazio principal() {\n" + corpo + " }\n}\n";
}

TEST(SemanticoChamada, FuncaoNaoDeclarada) {
    esperarErro(comPrincipal("  escreva(dobro(2));\n").c_str(), 5, "funcao 'dobro' nao declarada");
}

TEST(SemanticoChamada, Aridade) {
    esperarErro(comPrincipal("  escreva(soma(1, 2, 3));\n").c_str(), 5,
                "funcao 'soma' espera 2 argumento(s), mas recebeu 3");
}

TEST(SemanticoChamada, TipoDoArgumento) {
    esperarErro(comPrincipal("  escreva(soma(1, \"dois\"));\n").c_str(), 5,
                "argumento 2 de 'soma' deve ser do tipo inteiro (recebeu cadeia)");
}

TEST(SemanticoChamada, VazioEmExpressao) {
    esperarErro(comPrincipal("  inteiro x = nada();\n").c_str(), 5,
                "funcao 'nada' e vazio e nao devolve valor");
}

TEST(SemanticoChamada, VariavelChamadaComoFuncao) {
    esperarErro(comPrincipal("  inteiro x = 1;\n  x();\n").c_str(), 6, "'x' nao e uma funcao");
}

TEST(SemanticoChamada, FuncaoUsadaComoVariavel) {
    esperarErro(comPrincipal("  inteiro x = soma + 1;\n").c_str(), 5,
                "'soma' e uma funcao e deve ser chamada");
}

TEST(SemanticoChamada, PrincipalNaoPodeSerChamada) {
    esperarErro(comPrincipal("  principal();\n").c_str(), 5, "principal nao pode ser chamada");
}

// ---- retorne -----------------------------------------------------------

TEST(SemanticoRetorne, TipoIncompativel) {
    esperarErro(
        "programa {\n funcao vazio principal() { }\n"
        " funcao inteiro f() {\n  retorne \"x\";\n }\n}\n",
        4, "retorno da funcao 'f' deve ser do tipo inteiro (recebeu cadeia)");
}

TEST(SemanticoRetorne, SemValorEmFuncaoComTipo) {
    esperarErro(
        "programa {\n funcao vazio principal() { }\n"
        " funcao real f() {\n  retorne;\n }\n}\n",
        4, "funcao 'f' deve retornar um valor do tipo real");
}

TEST(SemanticoRetorne, ValorEmFuncaoVazio) {
    esperarErro(
        "programa {\n funcao vazio principal() {\n  retorne 1;\n }\n}\n", 3,
        "funcao 'principal' e vazio e nao pode retornar valor");
}

TEST(SemanticoRetorne, CaminhoSemRetorne) {
    esperarErro(
        "programa {\n funcao vazio principal() { }\n"
        " funcao inteiro sinal(inteiro x) {\n  se (x > 0) { retorne 1; }\n }\n}\n",
        3, "funcao 'sinal' do tipo inteiro pode terminar sem 'retorne'");
}

TEST(SemanticoRetorne, ComValorForaDeFuncao) {
    esperarErro("programa {\n retorne 2;\n}\n", 2, "'retorne' com valor fora de funcao");
}

// ---- desvios -----------------------------------------------------------

TEST(SemanticoDesvio, PareForaDeLaco) {
    esperarErro("programa {\n se (verdadeiro) {\n  pare;\n }\n}\n", 3,
                "'pare' so pode ser usado dentro de um laco");
}

TEST(SemanticoDesvio, ContinueForaDeLaco) {
    esperarErro("programa {\n continue;\n}\n", 2, "'continue' so pode ser usado dentro de um laco");
}

TEST(SemanticoDesvio, PareDentroDeLacoAninhadoEmSe) {
    esperarValido("programa {\n enquanto (verdadeiro) {\n  se (verdadeiro) { pare; }\n }\n}\n");
}

// ---- varios erros de uma vez -------------------------------------------

TEST(SemanticoRelatorio, RelataTodosOsErros) {
    const Resultado r = analisar(
        "programa {\n"
        " inteiro a = \"x\";\n"
        " pare;\n"
        " b = 1;\n"
        "}\n");
    EXPECT_EQ(r.erros, 3) << r.mensagens;
}

}  // namespace
