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

}  // namespace

