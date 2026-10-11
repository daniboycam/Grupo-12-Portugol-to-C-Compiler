%{
#include <iostream>
#include <string>
#include <cstdlib>
#include <cerrno>
#include <climits>

#if __has_include("PortugolToC.tab.hpp")
#include "PortugolToC.tab.hpp"
#elif __has_include("PortugolToC.tab.h")
#include "PortugolToC.tab.h"
#elif __has_include("y.tab.h")
#include "y.tab.h"
#elif __has_include("parser.tab.hpp")
#include "parser.tab.hpp"
#else
#include "portugol/tokens.hpp"
YYSTYPE yylval;
#endif

using namespace std;

// Copia o lexema para memoria propria
static char* duplicarLexema(const char* texto) {
    const size_t tamanho = strlen(texto) + 1;
    char* copia = new char[tamanho];
    memcpy(copia, texto, tamanho);
    return copia;
}

static string conteudo;
static int linhaLiteral = 0;

static char valorEscape(char letra) {
    switch (letra) {
        case 'n':  return '\n';
        case 't':  return '\t';
        case '\\': return '\\';
        case '"':  return '"';
        case '\'': return '\'';
        case '0':  return '\0';
        default:   return 0;
    }
}

static void erroLexicoNaLinha(int linha, const string& mensagem) {
    cerr << "erro lexico na linha " << linha << ": " << mensagem << "\n";
}

static void erroLexico(const string& mensagem) {
    erroLexicoNaLinha(yylineno, mensagem);
}
%}

%option noyywrap
%option yylineno
%option nounput
%option noinput

/* Estados exclusivos */
%x STRING
%x CHAR
%x COMMENT_BLOCO

/* Definicoes regulares */
DIGITO    [0-9]
LETRA     [A-Za-z]
ID        ({LETRA}|_)({LETRA}|{DIGITO}|_)*
INTEIRO   {DIGITO}+
EXPOENTE  [eE][+-]?{DIGITO}+
REAL      {DIGITO}+"."{DIGITO}+{EXPOENTE}?|{DIGITO}+{EXPOENTE}
REAL_MAL  {DIGITO}+"."|{DIGITO}+[eE][+-]?
NUM_MAL   {DIGITO}+({LETRA}|_)({LETRA}|{DIGITO}|_)*
ESPACO    [ \t]+
NOVA_LINHA \n
COMENTARIO_LINHA "//"[^\n]*

%%

    /* Separadores */
{ESPACO}             { /* ignora */ }
{NOVA_LINHA}         { /* ignora */ }
{COMENTARIO_LINHA}   { /* ignora */ }

    /* Comentario em bloco */
"/*"                 { linhaLiteral = yylineno; BEGIN(COMMENT_BLOCO); }
<COMMENT_BLOCO>"*/"  { BEGIN(INITIAL); }
<COMMENT_BLOCO>\n    { }
<COMMENT_BLOCO><<EOF>> { erroLexicoNaLinha(linhaLiteral, "comentario de bloco nao fechado no fim do arquivo"); BEGIN(INITIAL); return 0; }
<COMMENT_BLOCO>.     { }

    /* Palavras reservadas */
"programa"           { return KW_PROGRAMA; }
"funcao"             { return KW_FUNCAO; }
"retorne"            { return KW_RETORNE; }

"inteiro"            { return KW_INTEIRO; }
"real"               { return KW_REAL; }
"cadeia"             { return KW_CADEIA; }
"caracter"           { return KW_CARACTER; }
"logico"             { return KW_LOGICO; }
"vazio"              { return KW_VAZIO; }

"se"                 { return KW_SE; }
"senao"              { return KW_SENAO; }

"enquanto"           { return KW_ENQUANTO; }
"para"               { return KW_PARA; }
"pare"               { return KW_PARE; }
"continue"           { return KW_CONTINUE; }

"verdadeiro"         { return KW_VERDADEIRO; }
"falso"              { return KW_FALSO; }

"leia"               { return KW_LEIA; }
"escreva"            { return KW_ESCREVA; }

    /* Identificadores */
{ID}                 { yylval.str = duplicarLexema(yytext); return IDENTIFICADOR; }

    /* Literais numericos */
{REAL}               {
                       errno = 0;
                       const double valor = strtod(yytext, nullptr);
                       if (errno == ERANGE) {
                           erroLexico(string("literal real fora da faixa: ") + yytext);
                       }
                       yylval.fval = valor;
                       return NUM_REAL;
                     }

{INTEIRO}            {
                       errno = 0;
                       const long valor = strtol(yytext, nullptr, 10);
                       if (errno == ERANGE || valor > INT_MAX) {
                           erroLexico(string("literal inteiro fora da faixa: ") + yytext);
                           yylval.ival = 0;
                       } else {
                           yylval.ival = static_cast<int>(valor);
                       }
                       return NUM_INT;
                     }

{REAL_MAL}           { erroLexico(string("literal real mal formado: ") + yytext); }
{NUM_MAL}            { erroLexico(string("identificador nao pode comecar com digito: ") + yytext); }

    /* Cadeia */
\"                   { conteudo.clear(); linhaLiteral = yylineno; BEGIN(STRING); }

<STRING>\\.          {
                       const char valor = valorEscape(yytext[1]);
                       if (valor == 0 && yytext[1] != '0') {
                           erroLexico(string("sequencia de escape desconhecida: \\") + yytext[1]);
                       } else {
                           conteudo += valor;
                       }
                     }
<STRING>\"           {
                       yylval.str = duplicarLexema(conteudo.c_str());
                       BEGIN(INITIAL);
                       return LIT_STRING;
                     }
<STRING>\n           { erroLexicoNaLinha(linhaLiteral, "cadeia nao fechada"); BEGIN(INITIAL); }
<STRING><<EOF>>      { erroLexicoNaLinha(linhaLiteral, "cadeia nao fechada no fim do arquivo"); BEGIN(INITIAL); return 0; }
<STRING>.            { conteudo += yytext[0]; }

    /* Caracter */
'                    { conteudo.clear(); linhaLiteral = yylineno; BEGIN(CHAR); }

<CHAR>\\.            {
                       const char valor = valorEscape(yytext[1]);
                       if (valor == 0 && yytext[1] != '0') {
                           erroLexico(string("sequencia de escape desconhecida: \\") + yytext[1]);
                       } else {
                           conteudo += valor;
                       }
                     }
<CHAR>'              {
                       BEGIN(INITIAL);
                       if (conteudo.size() != 1) {
                           erroLexico("literal caracter deve conter exatamente um caractere");
                       } else {
                           yylval.cval = conteudo[0];
                           return LIT_CHAR;
                       }
                     }
<CHAR>\n             { erroLexicoNaLinha(linhaLiteral, "literal caracter nao fechado"); BEGIN(INITIAL); }
<CHAR><<EOF>>        { erroLexicoNaLinha(linhaLiteral, "literal caracter nao fechado no fim do arquivo"); BEGIN(INITIAL); return 0; }
<CHAR>.              { conteudo += yytext[0]; }

    /* Atribuicao composta */
"<<="                { return ATRIB_DESLOCA_ESQ; }
">>="                { return ATRIB_DESLOCA_DIR; }
"+="                 { return ATRIB_MAIS; }
"-="                 { return ATRIB_MENOS; }
"*="                 { return ATRIB_VEZES; }
"/="                 { return ATRIB_DIVIDIDO; }
"%="                 { return ATRIB_RESTO; }
"&="                 { return ATRIB_BIT_E; }
"|="                 { return ATRIB_BIT_OU; }
"^="                 { return ATRIB_BIT_XOU; }

    /* Operadores compostos */
"=="                 { return IGUAL; }
"!="                 { return DIFERENTE; }
"<="                 { return MENOR_IGUAL; }
">="                 { return MAIOR_IGUAL; }
"&&"                 { return E; }
"||"                 { return OU; }
"<<"                 { return DESLOCA_ESQ; }
">>"                 { return DESLOCA_DIR; }

    /* Operadores simples */
"+"                  { return MAIS; }
"-"                  { return MENOS; }
"*"                  { return VEZES; }
"/"                  { return DIVIDIDO; }
"%"                  { return RESTO; }
"="                  { return ATRIBUICAO; }
"<"                  { return MENOR; }
">"                  { return MAIOR; }
"!"                  { return NAO; }
"&"                  { return BIT_E; }
"|"                  { return BIT_OU; }
"^"                  { return BIT_XOU; }
"~"                  { return BIT_NAO; }

    /* Delimitadores */
"("                  { return ABRE_PAREN; }
")"                  { return FECHA_PAREN; }
"{"                  { return ABRE_CHAVE; }
"}"                  { return FECHA_CHAVE; }
"["                  { return ABRE_COLCHETE; }
"]"                  { return FECHA_COLCHETE; }
","                  { return VIRGULA; }
";"                  { return PONTO_VIRGULA; }

    /* Erro lexico */
.                    { erroLexico(string("caractere inesperado: ") + yytext); }

%%