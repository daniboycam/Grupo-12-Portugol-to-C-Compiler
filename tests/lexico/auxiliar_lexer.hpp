#pragma once
#include <string>
#include <vector>

// O scanner (portugolToC.l) usa o cabeçalho gerado pelo Bison. Os testes precisam
// usar o MESMO cabeçalho, senão os códigos dos tokens divergem.
// ATENÇÃO: o Bison define macros com os nomes dos tokens (E, OU, NAO...). Inclua os
// cabeçalhos padrão e o gtest ANTES deste arquivo.
#if __has_include("PortugolToC.tab.hpp")
#include "PortugolToC.tab.hpp"
#else
#include "portugol/tokens.hpp"
#endif

struct Token {
    int codigo = 0;
    std::string lexema;   // yytext (em cadeia e caracter é só a aspa de fechamento)
    int linha = 0;        // yylineno depois do token
    std::string str;      // IDENTIFICADOR e LIT_STRING (yylval.str)
    int ival = 0;         // NUM_INT
    double fval = 0.0;    // NUM_REAL
    char cval = 0;        // LIT_CHAR
};

// Se `erros` não for nulo, recebe o que o scanner escreveu em stderr.
std::vector<Token> tokenizar(const std::string& fonte, std::string* erros = nullptr);
std::string nome_do_token(int codigo);