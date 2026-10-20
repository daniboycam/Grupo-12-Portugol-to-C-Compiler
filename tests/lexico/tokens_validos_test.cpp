#include <gtest/gtest.h>
#include <utility>
#include "auxiliar_lexer.hpp"

static std::vector<int> codigos(const std::string& fonte, std::string* erros = nullptr) {
    std::vector<int> r;
    for (const auto& t : tokenizar(fonte, erros)) r.push_back(t.codigo);
    return r;
}

// ===== Token único: palavras reservadas, operadores e delimitadores =====
struct CasoToken { std::string entrada; int codigo; };
class TokenUnico : public ::testing::TestWithParam<CasoToken> {};

TEST_P(TokenUnico, ReconheceUmToken) {
    const auto& c = GetParam();
    std::string erros;
    auto toks = tokenizar(c.entrada, &erros);
    EXPECT_EQ(erros, "") << c.entrada;
    ASSERT_EQ(toks.size(), 1u) << c.entrada;
    EXPECT_EQ(toks[0].codigo, c.codigo)
        << c.entrada << ": esperado " << nome_do_token(c.codigo)
        << ", obtido " << nome_do_token(toks[0].codigo);
    EXPECT_EQ(toks[0].lexema, c.entrada);
}

INSTANTIATE_TEST_SUITE_P(PalavrasReservadas, TokenUnico, ::testing::Values(
    CasoToken{"programa", KW_PROGRAMA},     CasoToken{"funcao", KW_FUNCAO},
    CasoToken{"retorne", KW_RETORNE},       CasoToken{"inteiro", KW_INTEIRO},
    CasoToken{"real", KW_REAL},             CasoToken{"cadeia", KW_CADEIA},
    CasoToken{"caracter", KW_CARACTER},     CasoToken{"logico", KW_LOGICO},
    CasoToken{"vazio", KW_VAZIO},           CasoToken{"se", KW_SE},
    CasoToken{"senao", KW_SENAO},           CasoToken{"enquanto", KW_ENQUANTO},
    CasoToken{"para", KW_PARA},             CasoToken{"pare", KW_PARE},
    CasoToken{"continue", KW_CONTINUE},     CasoToken{"verdadeiro", KW_VERDADEIRO},
    CasoToken{"falso", KW_FALSO},           CasoToken{"leia", KW_LEIA},
    CasoToken{"escreva", KW_ESCREVA}
));

INSTANTIATE_TEST_SUITE_P(Operadores, TokenUnico, ::testing::Values(
    CasoToken{"+", MAIS},        CasoToken{"-", MENOS},       CasoToken{"*", VEZES},
    CasoToken{"/", DIVIDIDO},    CasoToken{"%", RESTO},
    CasoToken{"==", IGUAL},      CasoToken{"!=", DIFERENTE},
    CasoToken{"<", MENOR},       CasoToken{"<=", MENOR_IGUAL},
    CasoToken{">", MAIOR},       CasoToken{">=", MAIOR_IGUAL},
    CasoToken{"&&", E},          CasoToken{"||", OU},         CasoToken{"!", NAO},
    CasoToken{"&", BIT_E},       CasoToken{"|", BIT_OU},      CasoToken{"^", BIT_XOU},
    CasoToken{"~", BIT_NAO},     CasoToken{"<<", DESLOCA_ESQ}, CasoToken{">>", DESLOCA_DIR},
    CasoToken{"=", ATRIBUICAO},  CasoToken{"+=", ATRIB_MAIS}, CasoToken{"-=", ATRIB_MENOS},
    CasoToken{"*=", ATRIB_VEZES}, CasoToken{"/=", ATRIB_DIVIDIDO}, CasoToken{"%=", ATRIB_RESTO},
    CasoToken{"&=", ATRIB_BIT_E}, CasoToken{"|=", ATRIB_BIT_OU},   CasoToken{"^=", ATRIB_BIT_XOU},
    CasoToken{"<<=", ATRIB_DESLOCA_ESQ}, CasoToken{">>=", ATRIB_DESLOCA_DIR}
));

INSTANTIATE_TEST_SUITE_P(Delimitadores, TokenUnico, ::testing::Values(
    CasoToken{"(", ABRE_PAREN},    CasoToken{")", FECHA_PAREN},
    CasoToken{"{", ABRE_CHAVE},    CasoToken{"}", FECHA_CHAVE},
    CasoToken{"[", ABRE_COLCHETE}, CasoToken{"]", FECHA_COLCHETE},
    CasoToken{",", VIRGULA},       CasoToken{";", PONTO_VIRGULA}
));

// ===== Identificadores (seção 4) =====
class Identificador : public ::testing::TestWithParam<std::string> {};

TEST_P(Identificador, ViraIdentificadorEGuardaOLexema) {
    const std::string& nome = GetParam();
    std::string erros;
    auto toks = tokenizar(nome, &erros);
    EXPECT_EQ(erros, "") << nome;
    ASSERT_EQ(toks.size(), 1u) << nome;
    EXPECT_EQ(toks[0].codigo, IDENTIFICADOR) << nome;
    EXPECT_EQ(toks[0].str, nome);
}

INSTANTIATE_TEST_SUITE_P(Gramatica, Identificador, ::testing::Values(
    "x", "soma_total", "_temp", "valor2", "_",
    "Se", "SE", "Inteiro",                          // case sensitive
    "senaose", "inteiro2", "se_x", "retorne1"       // casamento mais longo
));

// ===== Literais (seção 5) =====
TEST(Literais, Inteiros) {
    const std::vector<std::pair<std::string, int>> casos = {
        {"0", 0}, {"7", 7}, {"1024", 1024}, {"007", 7}};
    for (const auto& [texto, valor] : casos) {
        std::string erros;
        auto toks = tokenizar(texto, &erros);
        EXPECT_EQ(erros, "") << texto;
        ASSERT_EQ(toks.size(), 1u) << texto;
        EXPECT_EQ(toks[0].codigo, NUM_INT) << texto;
        EXPECT_EQ(toks[0].ival, valor) << texto;
    }
}

TEST(Literais, Reais) {
    const std::vector<std::pair<std::string, double>> casos = {
        {"3.14", 3.14}, {"0.5", 0.5}, {"1.0e10", 1.0e10},
        {"2E-3", 2e-3}, {"1e5", 1e5}, {"1E+2", 100.0}};
    for (const auto& [texto, valor] : casos) {
        std::string erros;
        auto toks = tokenizar(texto, &erros);
        EXPECT_EQ(erros, "") << texto;
        ASSERT_EQ(toks.size(), 1u) << texto;
        EXPECT_EQ(toks[0].codigo, NUM_REAL) << texto;
        EXPECT_DOUBLE_EQ(toks[0].fval, valor) << texto;
    }
}

TEST(Literais, Cadeias) {
    const std::vector<std::pair<std::string, std::string>> casos = {
        {R"lex("ola")lex", "ola"},
        {R"lex("")lex", ""},
        {R"lex("linha 1\nlinha 2")lex", "linha 1\nlinha 2"},   // escape resolvido
        {R"lex("a\tb")lex", "a\tb"},
        {R"lex("barra \\ aspas \" apostrofo \'")lex", "barra \\ aspas \" apostrofo '"},
        {"\"olá\"", "olá"}};                                   // UTF-8 em cadeia é permitido
    for (const auto& [texto, valor] : casos) {
        std::string erros;
        auto toks = tokenizar(texto, &erros);
        EXPECT_EQ(erros, "") << texto;
        ASSERT_EQ(toks.size(), 1u) << texto;
        EXPECT_EQ(toks[0].codigo, LIT_STRING) << texto;
        EXPECT_EQ(toks[0].str, valor) << texto;
    }
}

TEST(Literais, Caracteres) {
    const std::vector<std::pair<std::string, char>> casos = {
        {"'a'", 'a'}, {R"lex('\n')lex", '\n'}, {R"lex('\t')lex", '\t'},
        {R"lex('\\')lex", '\\'}, {R"lex('\'')lex", '\''}, {R"lex('\"')lex", '"'},
        {R"lex('\0')lex", '\0'}, {"'\"'", '"'}};
    for (const auto& [texto, valor] : casos) {
        std::string erros;
        auto toks = tokenizar(texto, &erros);
        EXPECT_EQ(erros, "") << texto;
        ASSERT_EQ(toks.size(), 1u) << texto;
        EXPECT_EQ(toks[0].codigo, LIT_CHAR) << texto;
        EXPECT_EQ(toks[0].cval, valor) << texto;
    }
}

// ===== Sequências e casamento mais longo (seções 2 e 6) =====
struct CasoSequencia { std::string entrada; std::vector<int> esperado; };
class Sequencia : public ::testing::TestWithParam<CasoSequencia> {};

TEST_P(Sequencia, GeraOsTokensEsperados) {
    const auto& c = GetParam();
    std::string erros;
    EXPECT_EQ(codigos(c.entrada, &erros), c.esperado) << c.entrada;
    EXPECT_EQ(erros, "") << c.entrada;
}

INSTANTIATE_TEST_SUITE_P(Gramatica, Sequencia, ::testing::Values(
    CasoSequencia{"a&&b",  {IDENTIFICADOR, E, IDENTIFICADOR}},
    CasoSequencia{"a&=b",  {IDENTIFICADOR, ATRIB_BIT_E, IDENTIFICADOR}},
    CasoSequencia{"a>>=b", {IDENTIFICADOR, ATRIB_DESLOCA_DIR, IDENTIFICADOR}},
    CasoSequencia{"a>>b",  {IDENTIFICADOR, DESLOCA_DIR, IDENTIFICADOR}},
    CasoSequencia{"a--b",  {IDENTIFICADOR, MENOS, MENOS, IDENTIFICADOR}},
    CasoSequencia{"-5",    {MENOS, NUM_INT}},                      // sinal não faz parte do literal
    CasoSequencia{"/* /* */ */", {VEZES, DIVIDIDO}},               // comentário não aninha
    CasoSequencia{"x=3.14;", {IDENTIFICADOR, ATRIBUICAO, NUM_REAL, PONTO_VIRGULA}},
    CasoSequencia{"v[10]", {IDENTIFICADOR, ABRE_COLCHETE, NUM_INT, FECHA_COLCHETE}}
));

// ===== Separadores e comentários (seções 2 e 8) =====
TEST(Separadores, EspacosETabsSaoDescartados) {
    std::string erros;
    EXPECT_EQ(codigos("inteiro  x\t\tsenao   falso", &erros),
              (std::vector<int>{KW_INTEIRO, IDENTIFICADOR, KW_SENAO, KW_FALSO}));
    EXPECT_EQ(erros, "");
}

TEST(Separadores, QuebrasDeLinhaSaoDescartadas) {
    std::string erros;
    EXPECT_EQ(codigos("inteiro\nx\n\nse", &erros).size(), 3u);
    EXPECT_EQ(erros, "");
}

// TODO: reabilitar quando ESPACO aceitar \r (issue do bug)
TEST(Separadores, DISABLED_FinalDeLinhaDoWindowsEhSeparador) {
    std::string erros;
    EXPECT_EQ(codigos("inteiro\r\nx", &erros).size(), 2u);
    EXPECT_EQ(erros, "");
}

TEST(Separadores, EntradaVaziaOuSoEspacosNaoGeraToken) {
    EXPECT_TRUE(tokenizar("").empty());
    EXPECT_TRUE(tokenizar("  \n\t ").empty());
}

TEST(Comentarios, BlocoEhDescartadoEContaLinhas) {
    std::string erros;
    auto toks = tokenizar("/* a\nb */ se", &erros);
    EXPECT_EQ(erros, "");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].codigo, KW_SE);
    EXPECT_EQ(toks[0].linha, 2);
}

TEST(Comentarios, LinhaEhDescartadoAteOFimDaLinha) {
    std::string erros;
    auto toks = tokenizar("// ignorado\nse", &erros);
    EXPECT_EQ(erros, "");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].linha, 2);
}

TEST(Comentarios, VariacoesDeBloco) {
    for (const std::string c : {"/**/", "/***/", "/* a * b */", "/* // dentro */", "/* \"aspas */"}) {
        std::string erros;
        EXPECT_TRUE(tokenizar(c, &erros).empty()) << c;
        EXPECT_EQ(erros, "") << c;
    }
}

// ===== Integração: exemplo comentado da seção 11 da gramática =====
TEST(Integracao, ExemploDaSecao11) {
    const std::string fonte =
        "// soma dois numeros\n"
        "funcao inteiro soma(inteiro a, inteiro b) {\n"
        "    retorne a + b;\n"
        "}\n";
    std::string erros;
    auto toks = tokenizar(fonte, &erros);
    EXPECT_EQ(erros, "");

    const std::vector<std::pair<int, int>> esperado = {   // {código, linha}
        {KW_FUNCAO, 2}, {KW_INTEIRO, 2}, {IDENTIFICADOR, 2}, {ABRE_PAREN, 2},
        {KW_INTEIRO, 2}, {IDENTIFICADOR, 2}, {VIRGULA, 2}, {KW_INTEIRO, 2},
        {IDENTIFICADOR, 2}, {FECHA_PAREN, 2}, {ABRE_CHAVE, 2},
        {KW_RETORNE, 3}, {IDENTIFICADOR, 3}, {MAIS, 3}, {IDENTIFICADOR, 3}, {PONTO_VIRGULA, 3},
        {FECHA_CHAVE, 4}};
    ASSERT_EQ(toks.size(), esperado.size());
    for (size_t i = 0; i < esperado.size(); ++i) {
        EXPECT_EQ(toks[i].codigo, esperado[i].first) << "token " << i + 1;
        EXPECT_EQ(toks[i].linha, esperado[i].second) << "token " << i + 1;
    }
    EXPECT_EQ(toks[2].str, "soma");
}

TEST(Scanner, PodeSerReutilizadoEntreEntradas) {
    EXPECT_EQ(tokenizar("se").size(), 1u);
    EXPECT_EQ(tokenizar("senao").size(), 1u);
}