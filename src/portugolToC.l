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

// Interface publica exportada pelo scanner gerado pelo Flex:
// - int yylex(): avanca a leitura em yyin e retorna o codigo do proximo token
//   (ou 0 ao atingir EOF). Preenche yylval para tokens com valor semantico.
// - FILE* yyin: fluxo de entrada lido pelo scanner (padrao: stdin).
// - char* yytext / int yyleng: lexema bruto e comprimento em bytes do token.
// - int yylineno: linha corrente (1-indexada, mantida por %option yylineno).
// - int yycolumn: coluna do proximo caractere a ser lido na linha corrente.

/// Aloca uma copia em heap do lexema reconhecido para entrega via yylval.str.
/// A posse do buffer (delete[]) passa para quem consome o token (parser/driver).
static char* duplicarLexema(const char* texto) {
    const size_t tamanho = strlen(texto) + 1;
    char* copia = new char[tamanho];
    memcpy(copia, texto, tamanho);
    return copia;
}

/// Conta caracteres (codepoints) em uma sequencia UTF-8: bytes de continuacao
/// (padrao 10xxxxxx) nao incrementam a contagem.
static int contarCaracteres(const char* texto, int bytes) {
    int n = 0;
    for (int i = 0; i < bytes; ++i)
        if ((static_cast<unsigned char>(texto[i]) & 0xC0) != 0x80) ++n;
    return n;
}

/// Acumulador para montar o conteudo de literais de cadeia e caracter nos
/// estados exclusivos STRING e CHAR.
static string conteudo;

/// Coluna corrente (1-indexada, em caracteres UTF-8) do cursor de leitura.
int yycolumn = 1;

/// Coluna onde comecou o token atual (atualizada antes de cada acao por YY_USER_ACTION).
static int colunaToken = 1;

/// Linha e coluna de abertura de um literal ou comentario de bloco, usadas
/// para reportar a posicao de inicio caso nao seja fechado.
static int linhaLiteral = 0;
static int colunaLiteral = 0;

/// Converte o caractere identificador de uma sequencia de escape (\n, \t, \\,
/// \", \', \0) para seu valor correspondente. Retorna 0 para escapes invalidos.
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

/// Emite em stderr uma mensagem padronizada de erro lexico na linha e coluna informadas.
static void erroLexicoNaPosicao(int linha, int coluna, const string& mensagem) {
    cerr << "Erro léxico [Linha " << linha << ", Coluna " << coluna << "]: " << mensagem << "\n";
}

/// Emite em stderr um erro lexico na posicao (yylineno, colunaToken) do token atual.
static void erroLexico(const string& mensagem) {
    erroLexicoNaPosicao(yylineno, colunaToken, mensagem);
}

/// Executado antes de cada acao do Flex: registra a coluna inicial do token e
/// avanca yycolumn pela quantidade de caracteres UTF-8 do lexema lido.
#define YY_USER_ACTION \
    colunaToken = yycolumn; \
    yycolumn += contarCaracteres(yytext, yyleng);
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
ESPACO    [ \t\r]+
NOVA_LINHA \n
COMENTARIO_LINHA "//"[^\n]*

%%

    /* Separadores */
{ESPACO}             { /* ignora */ }
{NOVA_LINHA}         { yycolumn = 1; }
{COMENTARIO_LINHA}   { /* ignora */ }

    /* Comentario em bloco */
"/*"                 { linhaLiteral = yylineno; colunaLiteral = colunaToken; BEGIN(COMMENT_BLOCO); }
<COMMENT_BLOCO>"*/"  { BEGIN(INITIAL); }
<COMMENT_BLOCO>\n    { yycolumn = 1; }
<COMMENT_BLOCO><<EOF>> { erroLexicoNaPosicao(linhaLiteral, colunaLiteral, "comentario de bloco nao fechado no fim do arquivo"); BEGIN(INITIAL); return 0; }
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
\"                   { conteudo.clear(); linhaLiteral = yylineno; colunaLiteral = colunaToken; BEGIN(STRING); }

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
<STRING>\n           { erroLexicoNaPosicao(linhaLiteral, colunaLiteral, "cadeia nao fechada"); yycolumn = 1; BEGIN(INITIAL); }
<STRING><<EOF>>      { erroLexicoNaPosicao(linhaLiteral, colunaLiteral, "cadeia nao fechada no fim do arquivo"); BEGIN(INITIAL); return 0; }
<STRING>.            { conteudo += yytext[0]; }

    /* Caracter */
'                    { conteudo.clear(); linhaLiteral = yylineno; colunaLiteral = colunaToken; BEGIN(CHAR); }

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
                           erroLexicoNaPosicao(linhaLiteral, colunaLiteral, "literal caracter deve conter exatamente um caractere");
                       } else {
                           yylval.cval = conteudo[0];
                           return LIT_CHAR;
                       }
                     }
<CHAR>\n             { erroLexicoNaPosicao(linhaLiteral, colunaLiteral, "literal caracter nao fechado"); yycolumn = 1; BEGIN(INITIAL); }
<CHAR><<EOF>>        { erroLexicoNaPosicao(linhaLiteral, colunaLiteral, "literal caracter nao fechado no fim do arquivo"); BEGIN(INITIAL); return 0; }
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
.                    { erroLexico(string("caractere inesperado '") + yytext + "'"); }

%%