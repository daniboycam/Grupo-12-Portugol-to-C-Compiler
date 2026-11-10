// Testes unitarios e de integracao para a Arvore Sintatica Abstrata (issue #36).
#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>

#include "portugol/ast.hpp"

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

    yyin = arquivo;
    yyrestart(arquivo);
    yylineno = 1;
    yycolumn = 1;

    const int resultado = yyparse();
    std::fclose(arquivo);
    yyin = nullptr;
    return resultado;
}

std::string capturarImpressaoAST(const NoAST* raiz) {
    testing::internal::CaptureStdout();
    imprimirAST(raiz);
    return testing::internal::GetCapturedStdout();
}

TEST(ConstrucaoManualAST, ExpressaoAritmeticaModeloSemana06) {
    // 2 + 3 * 4 -> (+ 2 (* 3 4))
    NoAST* mult = criarNoOp('*', criarNoNum(3), criarNoNum(4));
    NoAST* soma = criarNoOp('+', criarNoNum(2), mult);

    ASSERT_NE(soma, nullptr);
    EXPECT_EQ(soma->tipo, NO_OP);
    EXPECT_EQ(soma->op, "+");
    ASSERT_NE(soma->esq, nullptr);
    EXPECT_EQ(soma->esq->tipo, NO_NUM);
    EXPECT_EQ(soma->esq->valor, 2);
    ASSERT_NE(soma->dir, nullptr);
    EXPECT_EQ(soma->dir->tipo, NO_OP);
    EXPECT_EQ(soma->dir->op, "*");

    EXPECT_EQ(capturarImpressaoAST(soma), "(+ 2 (* 3 4))\n");
    liberarAST(soma);
}

TEST(ParserAST, PrecedenciaEAssociatividadeEmExpressoes) {
    ASSERT_EQ(analisar("programa { inteiro x = 2 + 3 * 4; y = (2 + 3) * 4; z = 10 - 3 - 2; }"), 0);
    ASSERT_NE(raizAST, nullptr);
    EXPECT_EQ(raizAST->tipo, NO_PROGRAMA);
    ASSERT_EQ(raizAST->filhos.size(), 3u);

    // 1) inteiro x = 2 + 3 * 4;
    const NoAST* decl = raizAST->filhos[0];
    EXPECT_EQ(decl->tipo, NO_DECL_VAR);
    EXPECT_EQ(decl->op, "inteiro");
    ASSERT_EQ(decl->filhos.size(), 1u);
    const NoAST* declX = decl->filhos[0];
    EXPECT_EQ(declX->tipo, NO_DECLARADOR);
    EXPECT_EQ(declX->nome, "x");
    EXPECT_EQ(astParaString(declX->esq), "(+ 2 (* 3 4))");

    // 2) y = (2 + 3) * 4;
    const NoAST* atribY = raizAST->filhos[1];
    EXPECT_EQ(atribY->tipo, NO_ATRIBUICAO);
    EXPECT_EQ(atribY->op, "=");
    EXPECT_EQ(astParaString(atribY->dir), "(* (+ 2 3) 4)");

    // 3) z = 10 - 3 - 2; (associatividade a esquerda)
    const NoAST* atribZ = raizAST->filhos[2];
    EXPECT_EQ(astParaString(atribZ->dir), "(- (- 10 3) 2)");

    liberarAST(raizAST);
    raizAST = nullptr;
}

TEST(ParserAST, DeclaracoesMultiplasVetoresELiterais) {
    const char* fonte =
        "programa {\n"
        "    inteiro a, b = 10, v[5];\n"
        "    real taxa = 2.5;\n"
        "    cadeia msg = \"ola\";\n"
        "    caracter ch = 'P';\n"
        "    logico ok = verdadeiro && !falso;\n"
        "}\n";

    ASSERT_EQ(analisar(fonte), 0);
    ASSERT_NE(raizAST, nullptr);
    ASSERT_EQ(raizAST->filhos.size(), 5u);

    EXPECT_EQ(astParaString(raizAST->filhos[0]), "(decl inteiro a, (b = 10), v[5])");
    EXPECT_EQ(astParaString(raizAST->filhos[1]), "(decl real (taxa = 2.5))");
    EXPECT_EQ(astParaString(raizAST->filhos[2]), "(decl cadeia (msg = \"ola\"))");
    EXPECT_EQ(astParaString(raizAST->filhos[3]), "(decl caracter (ch = 'P'))");
    EXPECT_EQ(astParaString(raizAST->filhos[4]), "(decl logico (ok = (&& verdadeiro (! falso))))");

    liberarAST(raizAST);
    raizAST = nullptr;
}

TEST(ParserAST, CondicionaisELacosImprimemEstruturaCompleta) {
    const char* fonte =
        "programa {\n"
        "    inteiro x = 0;\n"
        "    enquanto (x < 10) {\n"
        "        se (x == 5) {\n"
        "            pare;\n"
        "        } senao {\n"
        "            x += 1;\n"
        "        }\n"
        "    }\n"
        "}\n";

    ASSERT_EQ(analisar(fonte), 0);
    ASSERT_NE(raizAST, nullptr);

    const std::string saida = capturarImpressaoAST(raizAST);
    const std::string esperado =
        "(programa\n"
        "  (decl inteiro (x = 0))\n"
        "  (enquanto (< x 10)\n"
        "    (bloco\n"
        "      (se (== x 5)\n"
        "        (bloco\n"
        "          (pare)\n"
        "        )\n"
        "        (senao\n"
        "          (bloco\n"
        "            (+= x 1)\n"
        "          )\n"
        "        )\n"
        "      )\n"
        "    )\n"
        "  )\n"
        ")\n";

    EXPECT_EQ(saida, esperado);
    liberarAST(raizAST);
    raizAST = nullptr;
}

TEST(ParserAST, ErroSintaticoLimpaRaizAST) {
    testing::internal::CaptureStderr();
    const int res = analisar("programa { inteiro x = ; }");
    testing::internal::GetCapturedStderr();

    EXPECT_NE(res, 0);
    EXPECT_EQ(raizAST, nullptr);
}

TEST(ParserAST, FuncoesComParametrosTipoDeRetornoECorpo) {
    const char* fonte =
        "funcao inteiro soma(inteiro a, inteiro b) {\n"
        "    retorne a + b;\n"
        "}\n"
        "funcao vazio principal() {\n"
        "    retorne;\n"
        "}\n";

    ASSERT_EQ(analisar(fonte), 0);
    ASSERT_NE(raizAST, nullptr);
    ASSERT_EQ(raizAST->filhos.size(), 2u);

    const NoAST* soma = raizAST->filhos[0];
    ASSERT_EQ(soma->tipo, NO_FUNCAO);
    EXPECT_EQ(soma->nome, "soma");
    EXPECT_EQ(soma->op, "inteiro");
    ASSERT_EQ(soma->filhos.size(), 2u);
    EXPECT_EQ(soma->filhos[0]->tipo, NO_PARAMETRO);
    EXPECT_EQ(astParaString(soma->filhos[0]), "inteiro a");
    EXPECT_EQ(astParaString(soma->filhos[1]), "inteiro b");
    ASSERT_NE(soma->dir, nullptr);
    ASSERT_EQ(soma->dir->filhos.size(), 1u);
    EXPECT_EQ(astParaString(soma->dir->filhos[0]), "(retorne (+ a b))");

    const NoAST* principal = raizAST->filhos[1];
    EXPECT_EQ(principal->nome, "principal");
    EXPECT_EQ(principal->op, "vazio");
    EXPECT_TRUE(principal->filhos.empty());
    ASSERT_EQ(principal->dir->filhos.size(), 1u);
    EXPECT_EQ(astParaString(principal->dir->filhos[0]), "(retorne)");

    liberarAST(raizAST);
    raizAST = nullptr;
}

TEST(ParserAST, ChamadaEmExpressaoEComoComando) {
    const char* fonte =
        "funcao vazio principal() {\n"
        "    inteiro y = dobro(3) + 1;\n"
        "    inteiro z = agora();\n"
        "    mostrar(y, z * 2);\n"
        "}\n";

    ASSERT_EQ(analisar(fonte), 0);
    ASSERT_NE(raizAST, nullptr);
    const NoAST* corpo = raizAST->filhos[0]->dir;
    ASSERT_EQ(corpo->filhos.size(), 3u);

    // A chamada fica dentro da expressao, com os argumentos, e nao vira um
    // identificador comum.
    EXPECT_EQ(astParaString(corpo->filhos[0]), "(decl inteiro (y = (+ (chamada dobro 3) 1)))");
    EXPECT_EQ(astParaString(corpo->filhos[1]), "(decl inteiro (z = (chamada agora)))");

    const NoAST* comando = corpo->filhos[2];
    ASSERT_EQ(comando->tipo, NO_CHAMADA);
    EXPECT_EQ(comando->nome, "mostrar");
    ASSERT_EQ(comando->filhos.size(), 2u);
    EXPECT_EQ(astParaString(comando), "(chamada mostrar y (* z 2))");

    liberarAST(raizAST);
    raizAST = nullptr;
}

TEST(ParserAST, LeiaEEscreva) {
    const char* fonte =
        "programa {\n"
        "    inteiro x, v[3];\n"
        "    leia(x);\n"
        "    leia(v[2]);\n"
        "    escreva(\"x = \", x + 1, \"\\n\");\n"
        "}\n";

    ASSERT_EQ(analisar(fonte), 0);
    ASSERT_NE(raizAST, nullptr);
    ASSERT_EQ(raizAST->filhos.size(), 4u);

    EXPECT_EQ(raizAST->filhos[1]->tipo, NO_LEIA);
    EXPECT_EQ(astParaString(raizAST->filhos[1]), "(leia x)");
    EXPECT_EQ(astParaString(raizAST->filhos[2]), "(leia v[2])");

    const NoAST* escreva = raizAST->filhos[3];
    ASSERT_EQ(escreva->tipo, NO_ESCREVA);
    ASSERT_EQ(escreva->filhos.size(), 3u);
    EXPECT_EQ(astParaString(escreva), "(escreva \"x = \" (+ x 1) \"\\n\")");

    liberarAST(raizAST);
    raizAST = nullptr;
}

TEST(ParserAST, ProgramaComSubprogramasImprimeArvoreCompleta) {
    const char* fonte =
        "programa {\n"
        "    inteiro x = 10;\n"
        "    funcao inteiro principal() {\n"
        "        se (x >= 10) { escreva(\"OK\"); }\n"
        "        retorne 0;\n"
        "    }\n"
        "}\n";

    ASSERT_EQ(analisar(fonte), 0);
    ASSERT_NE(raizAST, nullptr);

    const std::string esperado =
        "(programa\n"
        "  (decl inteiro (x = 10))\n"
        "  (funcao inteiro principal ()\n"
        "    (bloco\n"
        "      (se (>= x 10)\n"
        "        (bloco\n"
        "          (escreva \"OK\")\n"
        "        )\n"
        "      )\n"
        "      (retorne 0)\n"
        "    )\n"
        "  )\n"
        ")\n";

    EXPECT_EQ(capturarImpressaoAST(raizAST), esperado);
    liberarAST(raizAST);
    raizAST = nullptr;
}

TEST(ParserAST, ListasNaoAceitamVirgulaSobrando) {
    const char* casos[] = {
        "funcao inteiro principal(, inteiro a) { retorne 0; }",
        "funcao inteiro principal(inteiro a,) { retorne 0; }",
        "programa { inteiro y = soma(, 1); }",
        "programa { inteiro y = soma(1, ); }",
    };
    for (const char* fonte : casos) {
        testing::internal::CaptureStderr();
        const int res = analisar(fonte);
        testing::internal::GetCapturedStderr();
        EXPECT_NE(res, 0) << "Deveria recusar: " << fonte;
        EXPECT_EQ(raizAST, nullptr) << fonte;
    }
}

}  // namespace
