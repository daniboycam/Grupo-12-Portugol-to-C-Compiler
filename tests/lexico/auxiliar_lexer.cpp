#include <gtest/gtest.h>
#include "auxiliar_lexer.hpp"

typedef struct yy_buffer_state* YY_BUFFER_STATE;
extern int yylex();
extern char* yytext;
extern int yylineno;
extern YY_BUFFER_STATE yy_scan_string(const char* str);
extern void yy_delete_buffer(YY_BUFFER_STATE buffer);

std::vector<Token> tokenizar(const std::string& fonte, std::string* erros) {
    yylineno = 1;                                  // yy_scan_string não reinicia a contagem
    testing::internal::CaptureStderr();            // erros léxicos saem em cerr
    YY_BUFFER_STATE buf = yy_scan_string(fonte.c_str());

    std::vector<Token> tokens;
    int codigo;
    while ((codigo = yylex()) != 0) {              // 0 = fim de arquivo
        Token t;
        t.codigo = codigo;
        t.lexema = yytext;
        t.linha = yylineno;
        switch (codigo) {
            case IDENTIFICADOR:
            case LIT_STRING:
                t.str = yylval.str ? yylval.str : "";
                delete[] yylval.str;               // o scanner aloca com new[]
                yylval.str = nullptr;
                break;
            case NUM_INT:  t.ival = yylval.ival; break;
            case NUM_REAL: t.fval = yylval.fval; break;
            case LIT_CHAR: t.cval = yylval.cval; break;
            default: break;
        }
        tokens.push_back(t);
    }

    yy_delete_buffer(buf);
    std::string saida = testing::internal::GetCapturedStderr();
    if (erros) *erros = saida;
    return tokens;
}

std::string nome_do_token(int codigo) { return nomeToken(codigo); }