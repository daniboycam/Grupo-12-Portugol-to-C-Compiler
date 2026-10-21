#include <gtest/gtest.h>
#include "auxiliar_lexer.hpp"

typedef struct yy_buffer_state* YY_BUFFER_STATE;
extern int yylex();
extern char* yytext;
extern int yylineno;
extern int yycolumn;
extern YY_BUFFER_STATE yy_scan_string(const char* str);
extern void yy_delete_buffer(YY_BUFFER_STATE buffer);

std::vector<Token> tokenizar(const std::string& fonte, std::string* erros) {
    yylineno = 1;                                  // yy_scan_string não reinicia a contagem
    yycolumn = 1;
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

std::string nome_do_token(int codigo) {
    switch (codigo) {
#define NOME(x) case x: return #x;
        NOME(KW_PROGRAMA) NOME(KW_FUNCAO) NOME(KW_RETORNE)
        NOME(KW_INTEIRO) NOME(KW_REAL) NOME(KW_CADEIA) NOME(KW_CARACTER)
        NOME(KW_LOGICO) NOME(KW_VAZIO) NOME(KW_SE) NOME(KW_SENAO)
        NOME(KW_ENQUANTO) NOME(KW_PARA) NOME(KW_PARE) NOME(KW_CONTINUE)
        NOME(KW_VERDADEIRO) NOME(KW_FALSO) NOME(KW_LEIA) NOME(KW_ESCREVA)
        NOME(IDENTIFICADOR) NOME(NUM_INT) NOME(NUM_REAL) NOME(LIT_STRING) NOME(LIT_CHAR)
        NOME(MAIS) NOME(MENOS) NOME(VEZES) NOME(DIVIDIDO) NOME(RESTO)
        NOME(IGUAL) NOME(DIFERENTE) NOME(MENOR_IGUAL) NOME(MENOR) NOME(MAIOR_IGUAL) NOME(MAIOR)
        NOME(E) NOME(OU) NOME(NAO)
        NOME(BIT_E) NOME(BIT_OU) NOME(BIT_XOU) NOME(BIT_NAO) NOME(DESLOCA_ESQ) NOME(DESLOCA_DIR)
        NOME(ATRIBUICAO) NOME(ATRIB_MAIS) NOME(ATRIB_MENOS) NOME(ATRIB_VEZES)
        NOME(ATRIB_DIVIDIDO) NOME(ATRIB_RESTO) NOME(ATRIB_BIT_E) NOME(ATRIB_BIT_OU)
        NOME(ATRIB_BIT_XOU) NOME(ATRIB_DESLOCA_ESQ) NOME(ATRIB_DESLOCA_DIR)
        NOME(ABRE_PAREN) NOME(FECHA_PAREN) NOME(ABRE_CHAVE) NOME(FECHA_CHAVE)
        NOME(ABRE_COLCHETE) NOME(FECHA_COLCHETE) NOME(VIRGULA) NOME(PONTO_VIRGULA)
#undef NOME
        default: return "DESCONHECIDO(" + std::to_string(codigo) + ")";
    }
}