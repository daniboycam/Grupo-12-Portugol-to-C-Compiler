#pragma once
#include <string>
#include <vector>

// Único ponto dos testes que inclui o cabeçalho de tokens.
#include "portugol/tokens.hpp"

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