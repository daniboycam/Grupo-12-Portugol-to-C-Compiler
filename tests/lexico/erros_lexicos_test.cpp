#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "auxiliar_lexer.hpp"

// Formato definido na issue #10. Se mudar, só esta função precisa mudar.
static std::string erro(int linha, int coluna, const std::string& mensagem) {
    return "Erro léxico [Linha " + std::to_string(linha) + ", Coluna " +
           std::to_string(coluna) + "]: " + mensagem + "\n";
}

static std::vector<int> codigos(const std::string& fonte, std::string* erros = nullptr) {
    std::vector<int> r;
    for (const auto& t : tokenizar(fonte, erros)) r.push_back(t.codigo);
    return r;
}

// ===== Cada caso gera exatamente um erro, com mensagem, linha e coluna certas (seção 9) =====
struct CasoErro { std::string nome, entrada, mensagem; int linha, coluna; };
void PrintTo(const CasoErro& c, std::ostream* os) { *os << c.nome; }

class ErroLexico : public ::testing::TestWithParam<CasoErro> {};

TEST_P(ErroLexico, ReportaMensagemLinhaEColuna) {
    const auto& c = GetParam();
    std::string erros;
    tokenizar(c.entrada, &erros);
    EXPECT_EQ(erros, erro(c.linha, c.coluna, c.mensagem));
}

INSTANTIATE_TEST_SUITE_P(Gramatica, ErroLexico, ::testing::Values(
    // caractere fora da linguagem
    CasoErro{"caractere_desconhecido", "inteiro x = 5 @;", "caractere inesperado '@'", 1, 15},
    CasoErro{"erro_na_terceira_linha", "x\ny\n@", "caractere inesperado '@'", 3, 1},
    CasoErro{"erro_apos_comentario_multilinha", "/* a\nb */ @", "caractere inesperado '@'", 2, 6},
    CasoErro{"ponto_solto_em_real", "1.2.3", "caractere inesperado '.'", 1, 4},
    CasoErro{"ponto_inicial", ".5", "caractere inesperado '.'", 1, 1},
    // cadeia (posição = abertura da cadeia)
    CasoErro{"cadeia_ate_fim_arquivo", "\"ola", "cadeia nao fechada no fim do arquivo", 1, 1},
    CasoErro{"cadeia_apos_tokens", "x = \"ola", "cadeia nao fechada no fim do arquivo", 1, 5},
    CasoErro{"cadeia_quebrada", "x\n\"ola\nse", "cadeia nao fechada", 2, 1},
    CasoErro{"escape_desconhecido", R"lex("\q")lex", R"lex(sequencia de escape desconhecida: \q)lex", 1, 2},
    CasoErro{"escape_no_meio_da_cadeia", R"lex("ab\q")lex", R"lex(sequencia de escape desconhecida: \q)lex", 1, 4},
    // caracter (posição = abertura do literal)
    CasoErro{"caracter_vazio", "''", "literal caracter deve conter exatamente um caractere", 1, 1},
    CasoErro{"caracter_com_dois", "'ab'", "literal caracter deve conter exatamente um caractere", 1, 1},
    CasoErro{"caracter_ate_fim_arquivo", "'a", "literal caracter nao fechado no fim do arquivo", 1, 1},
    CasoErro{"caracter_quebrado", "x\n'a\nse", "literal caracter nao fechado", 2, 1},
    // número
    CasoErro{"real_sem_decimal", "3.", "literal real mal formado: 3.", 1, 1},
    CasoErro{"real_apos_atribuicao", "x = 3.", "literal real mal formado: 3.", 1, 5},
    CasoErro{"real_expoente_vazio", "1e", "literal real mal formado: 1e", 1, 1},
    CasoErro{"real_expoente_so_sinal", "1e+", "literal real mal formado: 1e+", 1, 1},
    CasoErro{"digito_inicial", "2x", "identificador nao pode comecar com digito: 2x", 1, 1},
    CasoErro{"numero_colado_em_letras", "12abc", "identificador nao pode comecar com digito: 12abc", 1, 1},
    CasoErro{"digito_underscore_letra", "3_a", "identificador nao pode comecar com digito: 3_a", 1, 1},
    // comentário (posição = abertura do comentário)
    CasoErro{"comentario_nao_fechado", "/* abc", "comentario de bloco nao fechado no fim do arquivo", 1, 1},
    CasoErro{"comentario_multilinha_nao_fechado", "x\n/* a\nb", "comentario de bloco nao fechado no fim do arquivo", 2, 1}
));

// ===== Caracteres fora da linguagem (seção 9) =====
TEST(ErroLexicoCaractere, CaracteresForaDaLinguagem) {
    for (const std::string c : {"@", "#", "$", "?", ":", "`", "\\", "."}) {
        std::string erros;
        EXPECT_TRUE(codigos(c, &erros).empty()) << c;
        EXPECT_EQ(erros, erro(1, 1, "caractere inesperado '" + c + "'")) << c;
    }
}