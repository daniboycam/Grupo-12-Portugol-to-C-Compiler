#include <gtest/gtest.h>
#include <climits>
#include <string>
#include <vector>
#include <utility>
#include "auxiliar_lexer.hpp"

static std::vector<int> codigos(const std::string& fonte, std::string* erros = nullptr) {
    std::vector<int> r;
    for (const auto& t : tokenizar(fonte, erros)) r.push_back(t.codigo);
    return r;
}

// ============================================================================
// 1. Identificador e Número Colados (Casos-limite de adjacência)
// ============================================================================

struct CasoAdjacencia {
    std::string entrada;
    std::vector<int> esperado;
};

class AdjacenciaIdentificadorNumero : public ::testing::TestWithParam<CasoAdjacencia> {};

TEST_P(AdjacenciaIdentificadorNumero, DesmembraTokensCorretamente) {
    const auto& c = GetParam();
    std::string erros;
    EXPECT_EQ(codigos(c.entrada, &erros), c.esperado) << c.entrada;
    EXPECT_EQ(erros, "") << c.entrada;
}

INSTANTIATE_TEST_SUITE_P(CasosLimite, AdjacenciaIdentificadorNumero, ::testing::Values(
    // Identificador colado com operadores e inteiros
    CasoAdjacencia{"x+1",     {IDENTIFICADOR, MAIS, NUM_INT}},
    CasoAdjacencia{"1+x",     {NUM_INT, MAIS, IDENTIFICADOR}},
    CasoAdjacencia{"x-5",     {IDENTIFICADOR, MENOS, NUM_INT}},
    CasoAdjacencia{"5-x",     {NUM_INT, MENOS, IDENTIFICADOR}},
    CasoAdjacencia{"x*2",     {IDENTIFICADOR, VEZES, NUM_INT}},
    CasoAdjacencia{"2*x",     {NUM_INT, VEZES, IDENTIFICADOR}},
    CasoAdjacencia{"x/10",    {IDENTIFICADOR, DIVIDIDO, NUM_INT}},
    CasoAdjacencia{"10/x",    {NUM_INT, DIVIDIDO, IDENTIFICADOR}},
    CasoAdjacencia{"x%2",     {IDENTIFICADOR, RESTO, NUM_INT}},
    CasoAdjacencia{"100%x",   {NUM_INT, RESTO, IDENTIFICADOR}},

    // Atribuições e operadores relacionais com números
    CasoAdjacencia{"x=0",     {IDENTIFICADOR, ATRIBUICAO, NUM_INT}},
    CasoAdjacencia{"x==1",    {IDENTIFICADOR, IGUAL, NUM_INT}},
    CasoAdjacencia{"x!=0",    {IDENTIFICADOR, DIFERENTE, NUM_INT}},
    CasoAdjacencia{"x<10",    {IDENTIFICADOR, MENOR, NUM_INT}},
    CasoAdjacencia{"x<=10",   {IDENTIFICADOR, MENOR_IGUAL, NUM_INT}},
    CasoAdjacencia{"x>0",     {IDENTIFICADOR, MAIOR, NUM_INT}},
    CasoAdjacencia{"x>=0",    {IDENTIFICADOR, MAIOR_IGUAL, NUM_INT}},

    // Atribuições compostas com inteiros
    CasoAdjacencia{"x+=1",    {IDENTIFICADOR, ATRIB_MAIS, NUM_INT}},
    CasoAdjacencia{"x-=2",    {IDENTIFICADOR, ATRIB_MENOS, NUM_INT}},
    CasoAdjacencia{"x*=3",    {IDENTIFICADOR, ATRIB_VEZES, NUM_INT}},
    CasoAdjacencia{"x/=4",    {IDENTIFICADOR, ATRIB_DIVIDIDO, NUM_INT}},
    CasoAdjacencia{"x%=5",    {IDENTIFICADOR, ATRIB_RESTO, NUM_INT}},
    CasoAdjacencia{"x&=1",    {IDENTIFICADOR, ATRIB_BIT_E, NUM_INT}},
    CasoAdjacencia{"x|=2",    {IDENTIFICADOR, ATRIB_BIT_OU, NUM_INT}},
    CasoAdjacencia{"x^=3",    {IDENTIFICADOR, ATRIB_BIT_XOU, NUM_INT}},
    CasoAdjacencia{"x<<=4",   {IDENTIFICADOR, ATRIB_DESLOCA_ESQ, NUM_INT}},
    CasoAdjacencia{"x>>=5",   {IDENTIFICADOR, ATRIB_DESLOCA_DIR, NUM_INT}},

    // Adjacência com números reais
    CasoAdjacencia{"x+3.14",  {IDENTIFICADOR, MAIS, NUM_REAL}},
    CasoAdjacencia{"3.14+x",  {NUM_REAL, MAIS, IDENTIFICADOR}},
    CasoAdjacencia{"x*0.5",   {IDENTIFICADOR, VEZES, NUM_REAL}},
    CasoAdjacencia{"0.5*x",   {NUM_REAL, VEZES, IDENTIFICADOR}},
    CasoAdjacencia{"x==3.14", {IDENTIFICADOR, IGUAL, NUM_REAL}},

    // Operações aritméticas diretas entre números sem espaço
    CasoAdjacencia{"123+456", {NUM_INT, MAIS, NUM_INT}},
    CasoAdjacencia{"123-456", {NUM_INT, MENOS, NUM_INT}},
    CasoAdjacencia{"123*456", {NUM_INT, VEZES, NUM_INT}},
    CasoAdjacencia{"123/456", {NUM_INT, DIVIDIDO, NUM_INT}},
    CasoAdjacencia{"123%456", {NUM_INT, RESTO, NUM_INT}},
    CasoAdjacencia{"1.5+2.5", {NUM_REAL, MAIS, NUM_REAL}},
    CasoAdjacencia{"10-5.5",  {NUM_INT, MENOS, NUM_REAL}},

    // Adjacência com delimitadores
    CasoAdjacencia{"v[0]",    {IDENTIFICADOR, ABRE_COLCHETE, NUM_INT, FECHA_COLCHETE}},
    CasoAdjacencia{"v[10]",   {IDENTIFICADOR, ABRE_COLCHETE, NUM_INT, FECHA_COLCHETE}},
    CasoAdjacencia{"(123)",   {ABRE_PAREN, NUM_INT, FECHA_PAREN}},
    CasoAdjacencia{"{42}",    {ABRE_CHAVE, NUM_INT, FECHA_CHAVE}},
    CasoAdjacencia{"a,1,b,2;",{IDENTIFICADOR, VIRGULA, NUM_INT, VIRGULA, IDENTIFICADOR, VIRGULA, NUM_INT, PONTO_VIRGULA}}
));

TEST(IdentificadoresComDigitos, IdentificadorValidoComNumerosNoMeioEFim) {
    const std::vector<std::string> casos = {
        "_0", "_1", "_123", "x1", "var2", "soma100", "a1b2c3",
        "var_123_abc_456", "_temp1_val2_", "x_1_y_2_z_3"
    };
    for (const auto& id : casos) {
        std::string erros;
        auto toks = tokenizar(id, &erros);
        EXPECT_EQ(erros, "") << id;
        ASSERT_EQ(toks.size(), 1u) << id;
        EXPECT_EQ(toks[0].codigo, IDENTIFICADOR) << id;
        EXPECT_EQ(toks[0].str, id) << id;
    }
}

// ============================================================================
// 2. Operadores Compostos e Desambiguação de Prefixos (Longest Match)
// ============================================================================

struct CasoOperadorComposto {
    std::string entrada;
    std::vector<int> esperado;
};

class OperadoresColisao : public ::testing::TestWithParam<CasoOperadorComposto> {};

TEST_P(OperadoresColisao, ReconheceCorretamenteRegraDoCasamentoMaisLongo) {
    const auto& c = GetParam();
    std::string erros;
    EXPECT_EQ(codigos(c.entrada, &erros), c.esperado) << c.entrada;
    EXPECT_EQ(erros, "") << c.entrada;
}

INSTANTIATE_TEST_SUITE_P(CasosLimite, OperadoresColisao, ::testing::Values(
    // & vs && vs &=
    CasoOperadorComposto{"a&b",   {IDENTIFICADOR, BIT_E, IDENTIFICADOR}},
    CasoOperadorComposto{"a&&b",  {IDENTIFICADOR, E, IDENTIFICADOR}},
    CasoOperadorComposto{"a&=b",  {IDENTIFICADOR, ATRIB_BIT_E, IDENTIFICADOR}},
    CasoOperadorComposto{"a&&&b", {IDENTIFICADOR, E, BIT_E, IDENTIFICADOR}},
    CasoOperadorComposto{"a&&&&b",{IDENTIFICADOR, E, E, IDENTIFICADOR}},
    CasoOperadorComposto{"a&&=b", {IDENTIFICADOR, E, ATRIBUICAO, IDENTIFICADOR}},

    // | vs || vs |=
    CasoOperadorComposto{"a|b",   {IDENTIFICADOR, BIT_OU, IDENTIFICADOR}},
    CasoOperadorComposto{"a||b",  {IDENTIFICADOR, OU, IDENTIFICADOR}},
    CasoOperadorComposto{"a|=b",  {IDENTIFICADOR, ATRIB_BIT_OU, IDENTIFICADOR}},
    CasoOperadorComposto{"a|||b", {IDENTIFICADOR, OU, BIT_OU, IDENTIFICADOR}},
    CasoOperadorComposto{"a||||b",{IDENTIFICADOR, OU, OU, IDENTIFICADOR}},
    CasoOperadorComposto{"a||=b", {IDENTIFICADOR, OU, ATRIBUICAO, IDENTIFICADOR}},

    // ^ vs ^=
    CasoOperadorComposto{"a^b",   {IDENTIFICADOR, BIT_XOU, IDENTIFICADOR}},
    CasoOperadorComposto{"a^=b",  {IDENTIFICADOR, ATRIB_BIT_XOU, IDENTIFICADOR}},

    // < vs <= vs << vs <<=
    CasoOperadorComposto{"a<b",    {IDENTIFICADOR, MENOR, IDENTIFICADOR}},
    CasoOperadorComposto{"a<=b",   {IDENTIFICADOR, MENOR_IGUAL, IDENTIFICADOR}},
    CasoOperadorComposto{"a<<b",   {IDENTIFICADOR, DESLOCA_ESQ, IDENTIFICADOR}},
    CasoOperadorComposto{"a<<=b",  {IDENTIFICADOR, ATRIB_DESLOCA_ESQ, IDENTIFICADOR}},
    CasoOperadorComposto{"a<<<=b", {IDENTIFICADOR, DESLOCA_ESQ, MENOR_IGUAL, IDENTIFICADOR}},
    CasoOperadorComposto{"a<<<<b", {IDENTIFICADOR, DESLOCA_ESQ, DESLOCA_ESQ, IDENTIFICADOR}},
    CasoOperadorComposto{"a<==b",  {IDENTIFICADOR, MENOR_IGUAL, ATRIBUICAO, IDENTIFICADOR}},

    // > vs >= vs >> vs >>=
    CasoOperadorComposto{"a>b",    {IDENTIFICADOR, MAIOR, IDENTIFICADOR}},
    CasoOperadorComposto{"a>=b",   {IDENTIFICADOR, MAIOR_IGUAL, IDENTIFICADOR}},
    CasoOperadorComposto{"a>>b",   {IDENTIFICADOR, DESLOCA_DIR, IDENTIFICADOR}},
    CasoOperadorComposto{"a>>=b",  {IDENTIFICADOR, ATRIB_DESLOCA_DIR, IDENTIFICADOR}},
    CasoOperadorComposto{"a>>>=b", {IDENTIFICADOR, DESLOCA_DIR, MAIOR_IGUAL, IDENTIFICADOR}},
    CasoOperadorComposto{"a>>>>b", {IDENTIFICADOR, DESLOCA_DIR, DESLOCA_DIR, IDENTIFICADOR}},
    CasoOperadorComposto{"a>==b",  {IDENTIFICADOR, MAIOR_IGUAL, ATRIBUICAO, IDENTIFICADOR}},

    // = vs == vs != vs !== vs ===
    CasoOperadorComposto{"a=b",    {IDENTIFICADOR, ATRIBUICAO, IDENTIFICADOR}},
    CasoOperadorComposto{"a==b",   {IDENTIFICADOR, IGUAL, IDENTIFICADOR}},
    CasoOperadorComposto{"a!=b",   {IDENTIFICADOR, DIFERENTE, IDENTIFICADOR}},
    CasoOperadorComposto{"a!==b",  {IDENTIFICADOR, DIFERENTE, ATRIBUICAO, IDENTIFICADOR}},
    CasoOperadorComposto{"a===b",  {IDENTIFICADOR, IGUAL, ATRIBUICAO, IDENTIFICADOR}},
    CasoOperadorComposto{"a====b", {IDENTIFICADOR, IGUAL, IGUAL, IDENTIFICADOR}},

    // Sequências de + e - (não existem ++ e --)
    CasoOperadorComposto{"a++b",   {IDENTIFICADOR, MAIS, MAIS, IDENTIFICADOR}},
    CasoOperadorComposto{"a+++b",  {IDENTIFICADOR, MAIS, MAIS, MAIS, IDENTIFICADOR}},
    CasoOperadorComposto{"a--b",   {IDENTIFICADOR, MENOS, MENOS, IDENTIFICADOR}},
    CasoOperadorComposto{"a---b",  {IDENTIFICADOR, MENOS, MENOS, MENOS, IDENTIFICADOR}},
    CasoOperadorComposto{"a+-b",   {IDENTIFICADOR, MAIS, MENOS, IDENTIFICADOR}},
    CasoOperadorComposto{"a-+b",   {IDENTIFICADOR, MENOS, MAIS, IDENTIFICADOR}},
    CasoOperadorComposto{"a+-+b",  {IDENTIFICADOR, MAIS, MENOS, MAIS, IDENTIFICADOR}},

    // Sequências com * e %
    CasoOperadorComposto{"a**b",   {IDENTIFICADOR, VEZES, VEZES, IDENTIFICADOR}},
    CasoOperadorComposto{"a***b",  {IDENTIFICADOR, VEZES, VEZES, VEZES, IDENTIFICADOR}},
    CasoOperadorComposto{"a%%b",   {IDENTIFICADOR, RESTO, RESTO, IDENTIFICADOR}},

    // Operadores unários consecutivos
    CasoOperadorComposto{"!a",     {NAO, IDENTIFICADOR}},
    CasoOperadorComposto{"!!a",    {NAO, NAO, IDENTIFICADOR}},
    CasoOperadorComposto{"!!!a",   {NAO, NAO, NAO, IDENTIFICADOR}},
    CasoOperadorComposto{"~a",     {BIT_NAO, IDENTIFICADOR}},
    CasoOperadorComposto{"~~a",    {BIT_NAO, BIT_NAO, IDENTIFICADOR}},
    CasoOperadorComposto{"!~a",    {NAO, BIT_NAO, IDENTIFICADOR}},
    CasoOperadorComposto{"~!a",    {BIT_NAO, NAO, IDENTIFICADOR}},
    CasoOperadorComposto{"~-a",    {BIT_NAO, MENOS, IDENTIFICADOR}},
    CasoOperadorComposto{"!-a",    {NAO, MENOS, IDENTIFICADOR}}
));

// ============================================================================
// 3. Palavras Reservadas: Casos-limite de Prefixos e Case-Sensitivity
// ============================================================================

TEST(PalavrasReservadasCasosLimite, IdentificadorComPrefixoDePalavraReservada) {
    const std::vector<std::string> identificadores = {
        "programas", "programa_principal", "funcoes", "funcao_teste",
        "retornou", "retornei", "inteiros", "inteiro_valor",
        "reais", "real_val", "cadeias", "cadeia_txt",
        "caracteres", "caracter_c", "logicos", "logico_b",
        "vazios", "vazio_fn", "sem", "selo", "setor", "seja", "se_entao",
        "senaose", "senao_outro", "enquanto_loop", "enquantos",
        "parar", "parametro", "parada", "paredes", "parede", "parei",
        "continuar", "continua", "continuidade", "verdadeiros",
        "falsos", "falsidade", "leiam", "leitura", "escrever", "escrevas"
    };

    for (const auto& id : identificadores) {
        std::string erros;
        auto toks = tokenizar(id, &erros);
        EXPECT_EQ(erros, "") << id;
        ASSERT_EQ(toks.size(), 1u) << id;
        EXPECT_EQ(toks[0].codigo, IDENTIFICADOR)
            << id << " foi classificado incorretamente como " << nome_do_token(toks[0].codigo);
        EXPECT_EQ(toks[0].str, id) << id;
    }
}

TEST(PalavrasReservadasCasosLimite, CaseSensitivityRigida) {
    // Nenhuma variação maiúscula de palavra reservada pode virar palavra reservada
    const std::vector<std::string> maiusculas = {
        "Programa", "PROGRAMA", "proGrama",
        "Funcao", "FUNCAO", "funCao",
        "Retorne", "RETORNE",
        "Inteiro", "INTEIRO",
        "Real", "REAL",
        "Cadeia", "CADEIA",
        "Caracter", "CARACTER",
        "Logico", "LOGICO",
        "Vazio", "VAZIO",
        "Se", "SE", "sE",
        "Senao", "SENAO", "SeNao",
        "Enquanto", "ENQUANTO",
        "Para", "PARA",
        "Pare", "PARE",
        "Continue", "CONTINUE",
        "Verdadeiro", "VERDADEIRO",
        "Falso", "FALSO",
        "Leia", "LEIA",
        "Escreva", "ESCREVA"
    };

    for (const auto& palavra : maiusculas) {
        std::string erros;
        auto toks = tokenizar(palavra, &erros);
        EXPECT_EQ(erros, "") << palavra;
        ASSERT_EQ(toks.size(), 1u) << palavra;
        EXPECT_EQ(toks[0].codigo, IDENTIFICADOR)
            << palavra << " foi classificado incorretamente como " << nome_do_token(toks[0].codigo);
        EXPECT_EQ(toks[0].str, palavra) << palavra;
    }
}

TEST(PalavrasReservadasCasosLimite, UnderscorePrefixoComPalavraReservada) {
    for (const std::string id : {"_se", "_senao", "_para", "_inteiro", "_retorne", "_programa"}) {
        std::string erros;
        auto toks = tokenizar(id, &erros);
        EXPECT_EQ(erros, "") << id;
        ASSERT_EQ(toks.size(), 1u) << id;
        EXPECT_EQ(toks[0].codigo, IDENTIFICADOR) << id;
        EXPECT_EQ(toks[0].str, id) << id;
    }
}

// ============================================================================
// 4. Literais Numéricos: Casos-limite e Fronteiras
// ============================================================================

TEST(LiteraisNumericosCasosLimite, InteirosComMultiplosZerosPreservamBaseDecimal) {
    const std::vector<std::pair<std::string, int>> casos = {
        {"0", 0}, {"00", 0}, {"000", 0}, {"0000", 0},
        {"01", 1}, {"007", 7}, {"000123", 123},
        {std::to_string(INT_MAX), INT_MAX}
    };

    for (const auto& [texto, valor] : casos) {
        std::string erros;
        auto toks = tokenizar(texto, &erros);
        EXPECT_EQ(erros, "") << texto;
        ASSERT_EQ(toks.size(), 1u) << texto;
        EXPECT_EQ(toks[0].codigo, NUM_INT) << texto;
        EXPECT_EQ(toks[0].ival, valor) << texto;
    }
}

TEST(LiteraisNumericosCasosLimite, ReaisComExpoentePuroSemPonto) {
    // literal_real = ... | digito { digito } expoente ;
    const std::vector<std::pair<std::string, double>> casos = {
        {"1e2", 100.0},
        {"2E3", 2000.0},
        {"5e-1", 0.5},
        {"4E+2", 400.0},
        {"10e0", 10.0},
        {"0e0", 0.0},
        {"0E5", 0.0}
    };

    for (const auto& [texto, valor] : casos) {
        std::string erros;
        auto toks = tokenizar(texto, &erros);
        EXPECT_EQ(erros, "") << texto;
        ASSERT_EQ(toks.size(), 1u) << texto;
        EXPECT_EQ(toks[0].codigo, NUM_REAL) << texto;
        EXPECT_DOUBLE_EQ(toks[0].fval, valor) << texto;
    }
}

TEST(LiteraisNumericosCasosLimite, ReaisComPontoEExpoenteVariado) {
    const std::vector<std::pair<std::string, double>> casos = {
        {"0.0", 0.0},
        {"00.00", 0.0},
        {"0.000", 0.0},
        {"0.0e0", 0.0},
        {"1.234e2", 123.4},
        {"1.234e-2", 0.01234},
        {"12.34E+1", 123.4},
        {"0.00001", 0.00001},
        {"100000.0", 100000.0}
    };

    for (const auto& [texto, valor] : casos) {
        std::string erros;
        auto toks = tokenizar(texto, &erros);
        EXPECT_EQ(erros, "") << texto;
        ASSERT_EQ(toks.size(), 1u) << texto;
        EXPECT_EQ(toks[0].codigo, NUM_REAL) << texto;
        EXPECT_DOUBLE_EQ(toks[0].fval, valor) << texto;
    }
}

// ============================================================================
// 5. Literais de Cadeia e Caracter: Casos Especiais
// ============================================================================

TEST(LiteraisCadeiasCasosLimite, CadeiaComTodosOsEscapesValidosCombinados) {
    std::string erros;
    auto toks = tokenizar(R"lex("\n\t\\\"\'")lex", &erros);
    EXPECT_EQ(erros, "");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].codigo, LIT_STRING);
    EXPECT_EQ(toks[0].str, "\n\t\\\"'");
}

TEST(LiteraisCadeiasCasosLimite, EscapeNuloEmCadeia) {
    // Como yylval.str e duplicarLexema utilizam char* (C-string terminada em nulo),
    // o escape \0 atua como terminador da string em C++.
    std::string erros;
    auto toks = tokenizar(R"lex("\0")lex", &erros);
    EXPECT_EQ(erros, "");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].codigo, LIT_STRING);
    EXPECT_EQ(toks[0].str, "");
}

TEST(LiteraisCadeiasCasosLimite, CodigoEComentariosDentroDeCadeiaNaoGeramTokens) {
    const std::string entrada = R"lex("se (x > 0) { retorne 1; } // comentario /* bloco */")lex";
    std::string erros;
    auto toks = tokenizar(entrada, &erros);
    EXPECT_EQ(erros, "");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].codigo, LIT_STRING);
    EXPECT_EQ(toks[0].str, "se (x > 0) { retorne 1; } // comentario /* bloco */");
}

TEST(LiteraisCadeiasCasosLimite, CadeiasConsecutivasComEspaco) {
    std::string erros;
    auto toks = tokenizar("\"primeira\" \"segunda\"", &erros);
    EXPECT_EQ(erros, "");
    ASSERT_EQ(toks.size(), 2u);
    EXPECT_EQ(toks[0].codigo, LIT_STRING);
    EXPECT_EQ(toks[0].str, "primeira");
    EXPECT_EQ(toks[1].codigo, LIT_STRING);
    EXPECT_EQ(toks[1].str, "segunda");
}

TEST(LiteraisCaracterCasosLimite, SimbolosEDelimitadoresComoCaracter) {
    const std::vector<std::pair<std::string, char>> casos = {
        {"' '", ' '},
        {"'+'", '+'}, {"'-'", '-'}, {"'*'", '*'}, {"'/'", '/'},
        {"'='", '='}, {"';'", ';'}, {"'{'", '{'}, {"'('", '('}, {"'['", '['},
        {"'0'", '0'}, {"'9'", '9'}, {"'A'", 'A'}, {"'z'", 'z'},
        {"'_'", '_'}, {"'$'", '$'}, {"'@'", '@'}
    };

    for (const auto& [texto, c] : casos) {
        std::string erros;
        auto toks = tokenizar(texto, &erros);
        EXPECT_EQ(erros, "") << texto;
        ASSERT_EQ(toks.size(), 1u) << texto;
        EXPECT_EQ(toks[0].codigo, LIT_CHAR) << texto;
        EXPECT_EQ(toks[0].cval, c) << texto;
    }
}

TEST(LiteraisCaracterCasosLimite, CaracteresContiguos) {
    std::string erros;
    auto toks = tokenizar("'a''b''c'", &erros);
    EXPECT_EQ(erros, "");
    ASSERT_EQ(toks.size(), 3u);
    EXPECT_EQ(toks[0].cval, 'a');
    EXPECT_EQ(toks[1].cval, 'b');
    EXPECT_EQ(toks[2].cval, 'c');
}

// ============================================================================
// 6. Comentários: Casos-limite de Adjacência e Fronteiras
// ============================================================================

TEST(ComentariosCasosLimite, ComentarioDeLinhaNoFimDeArquivoSemQuebraDeLinha) {
    std::string erros;
    auto toks = tokenizar("// comentario sem newline no eof", &erros);
    EXPECT_EQ(erros, "");
    EXPECT_TRUE(toks.empty());
}

TEST(ComentariosCasosLimite, CodigoColadoAntesDeComentarioDeLinha) {
    std::string erros;
    auto toks = tokenizar("retorne;// fim de linha", &erros);
    EXPECT_EQ(erros, "");
    ASSERT_EQ(toks.size(), 2u);
    EXPECT_EQ(toks[0].codigo, KW_RETORNE);
    EXPECT_EQ(toks[1].codigo, PONTO_VIRGULA);
}

TEST(ComentariosCasosLimite, CodigoEComentarioDeBlocoColadosSemEspaco) {
    std::string erros;
    auto toks = tokenizar("retorne/*bloco*/1;", &erros);
    EXPECT_EQ(erros, "");
    ASSERT_EQ(toks.size(), 3u);
    EXPECT_EQ(toks[0].codigo, KW_RETORNE);
    EXPECT_EQ(toks[1].codigo, NUM_INT);
    EXPECT_EQ(toks[1].ival, 1);
    EXPECT_EQ(toks[2].codigo, PONTO_VIRGULA);
}

TEST(ComentariosCasosLimite, TokensSeparadosExclusivamentePorComentarios) {
    std::string erros;
    auto toks = tokenizar("a/*1*/+/*2*/b", &erros);
    EXPECT_EQ(erros, "");
    ASSERT_EQ(toks.size(), 3u);
    EXPECT_EQ(toks[0].codigo, IDENTIFICADOR);
    EXPECT_EQ(toks[1].codigo, MAIS);
    EXPECT_EQ(toks[2].codigo, IDENTIFICADOR);
}

TEST(ComentariosCasosLimite, MultiplosComentariosDeBlocoContiguos) {
    std::string erros;
    auto toks = tokenizar("/*1*//*2*//*3*/retorne;", &erros);
    EXPECT_EQ(erros, "");
    ASSERT_EQ(toks.size(), 2u);
    EXPECT_EQ(toks[0].codigo, KW_RETORNE);
    EXPECT_EQ(toks[1].codigo, PONTO_VIRGULA);
}

TEST(ComentariosCasosLimite, ComentarioDeBlocoComBarrasEComentarioDeLinhaDentro) {
    std::string erros;
    auto toks = tokenizar("/* ///// // isto nao fecha */se", &erros);
    EXPECT_EQ(erros, "");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].codigo, KW_SE);
}

TEST(ComentariosCasosLimite, ComentarioDeLinhaComBlocoDentroNaoAtivaBloco) {
    std::string erros;
    auto toks = tokenizar("// /* nao e bloco */\nse", &erros);
    EXPECT_EQ(erros, "");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].codigo, KW_SE);
    EXPECT_EQ(toks[0].linha, 2);
}

// ============================================================================
// 7. Expressões e Estruturas Sintáticas sem Espaços (Stress Test Léxico)
// ============================================================================

TEST(EstruturasComplexasSemEspaco, DeclaracaoDeVetorComInicializacao) {
    const std::string codigoFonte = "inteiro v[5]={1,2,3,4,5};";
    std::string erros;
    EXPECT_EQ(codigos(codigoFonte, &erros),
              (std::vector<int>{
                  KW_INTEIRO, IDENTIFICADOR, ABRE_COLCHETE, NUM_INT, FECHA_COLCHETE,
                  ATRIBUICAO, ABRE_CHAVE,
                  NUM_INT, VIRGULA, NUM_INT, VIRGULA, NUM_INT, VIRGULA, NUM_INT, VIRGULA, NUM_INT,
                  FECHA_CHAVE, PONTO_VIRGULA
              }));
    EXPECT_EQ(erros, "");
}

TEST(EstruturasComplexasSemEspaco, ExpressaoLogicaEBitwiseComplexa) {
    const std::string codigoFonte = "(x>0)&&(y<=10)||(z!=w)&(~c)^d;";
    std::string erros;
    EXPECT_EQ(codigos(codigoFonte, &erros),
              (std::vector<int>{
                  ABRE_PAREN, IDENTIFICADOR, MAIOR, NUM_INT, FECHA_PAREN,
                  E,
                  ABRE_PAREN, IDENTIFICADOR, MENOR_IGUAL, NUM_INT, FECHA_PAREN,
                  OU,
                  ABRE_PAREN, IDENTIFICADOR, DIFERENTE, IDENTIFICADOR, FECHA_PAREN,
                  BIT_E,
                  ABRE_PAREN, BIT_NAO, IDENTIFICADOR, FECHA_PAREN,
                  BIT_XOU, IDENTIFICADOR,
                  PONTO_VIRGULA
              }));
    EXPECT_EQ(erros, "");
}

TEST(EstruturasComplexasSemEspaco, LacoParaSemEspacos) {
    const std::string codigoFonte = "para(inteiro i=0;i<10;i+=1){}";
    std::string erros;
    EXPECT_EQ(codigos(codigoFonte, &erros),
              (std::vector<int>{
                  KW_PARA, ABRE_PAREN,
                  KW_INTEIRO, IDENTIFICADOR, ATRIBUICAO, NUM_INT, PONTO_VIRGULA,
                  IDENTIFICADOR, MENOR, NUM_INT, PONTO_VIRGULA,
                  IDENTIFICADOR, ATRIB_MAIS, NUM_INT,
                  FECHA_PAREN,
                  ABRE_CHAVE, FECHA_CHAVE
              }));
    EXPECT_EQ(erros, "");
}

TEST(EstruturasComplexasSemEspaco, CondicionalCompletaSemEspacos) {
    const std::string codigoFonte = "se(a==b){retorne verdadeiro;}senao{retorne falso;}";
    std::string erros;
    EXPECT_EQ(codigos(codigoFonte, &erros),
              (std::vector<int>{
                  KW_SE, ABRE_PAREN, IDENTIFICADOR, IGUAL, IDENTIFICADOR, FECHA_PAREN,
                  ABRE_CHAVE, KW_RETORNE, KW_VERDADEIRO, PONTO_VIRGULA, FECHA_CHAVE,
                  KW_SENAO,
                  ABRE_CHAVE, KW_RETORNE, KW_FALSO, PONTO_VIRGULA, FECHA_CHAVE
              }));
    EXPECT_EQ(erros, "");
}

TEST(EstruturasComplexasSemEspaco, RetornoDeNumeroNegativo) {
    // retorne-1; -> retorne, -, 1, ;
    const std::string codigoFonte = "retorne-1;";
    std::string erros;
    EXPECT_EQ(codigos(codigoFonte, &erros),
              (std::vector<int>{KW_RETORNE, MENOS, NUM_INT, PONTO_VIRGULA}));
    EXPECT_EQ(erros, "");
}

// ============================================================================
// 8. Rastreamento Preciso de Linhas com Múltiplas Quebras e CRLF
// ============================================================================

TEST(RastreamentoLinhas, ContagemCorretaAposVariasQuebrasECRLF) {
    const std::string codigoFonte =
        "inteiro a;\r\n"
        "\r\n"
        "/* bloco\r\n"
        "   linha 4 */\r\n"
        "real b;\n"
        "\n"
        "retorne 0;\n";

    std::string erros;
    auto toks = tokenizar(codigoFonte, &erros);
    EXPECT_EQ(erros, "");
    ASSERT_EQ(toks.size(), 9u);

    EXPECT_EQ(toks[0].codigo, KW_INTEIRO);    EXPECT_EQ(toks[0].linha, 1);
    EXPECT_EQ(toks[1].codigo, IDENTIFICADOR); EXPECT_EQ(toks[1].linha, 1);
    EXPECT_EQ(toks[2].codigo, PONTO_VIRGULA); EXPECT_EQ(toks[2].linha, 1);

    EXPECT_EQ(toks[3].codigo, KW_REAL);       EXPECT_EQ(toks[3].linha, 5);
    EXPECT_EQ(toks[4].codigo, IDENTIFICADOR); EXPECT_EQ(toks[4].linha, 5);
    EXPECT_EQ(toks[5].codigo, PONTO_VIRGULA); EXPECT_EQ(toks[5].linha, 5);

    EXPECT_EQ(toks[6].codigo, KW_RETORNE);    EXPECT_EQ(toks[6].linha, 7);
    EXPECT_EQ(toks[7].codigo, NUM_INT);       EXPECT_EQ(toks[7].linha, 7);
    EXPECT_EQ(toks[8].codigo, PONTO_VIRGULA); EXPECT_EQ(toks[8].linha, 7);
}
