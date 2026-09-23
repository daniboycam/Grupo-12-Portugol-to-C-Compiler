%{
#include <iostream>
#include <string>
#include <cstdlib>
#include <cerrno>
#include <climits>

// --- Ponte de inclusão dos tokens do Bison ---
// O Bison é o analisador sintático que define os códigos numéricos de cada token (ex: TOKEN_SE, TOKEN_INTEIRO).
// O Flex (analisador léxico) precisa desse cabeçalho (.h) para devolver o token correto ao Bison.
// As diretivas abaixo tentam incluir o cabeçalho automaticamente se ele já tiver sido gerado:
#if __has_include("PortugolToC.tab.h")
#include "PortugolToC.tab.h"
#elif __has_include("y.tab.h")
#include "y.tab.h"
#elif __has_include("parser.tab.hpp")
#include "parser.tab.hpp"
#else
// Enquanto o Bison nao existe, os codigos vem do cabecalho do proprio projeto
// e o scanner precisa definir o yylval que o parser definiria.
#include "portugol/tokens.hpp"
YYSTYPE yylval;
#endif

using namespace std;

// Copia o lexema para memoria propria: o yytext pertence ao Flex e e sobrescrito
// no proximo token. Quem consome o yylval.str e responsavel por liberar com delete[].
static char* duplicarLexema(const char* texto) {
    const size_t tamanho = strlen(texto) + 1;
    char* copia = new char[tamanho];
    memcpy(copia, texto, tamanho);
    return copia;
}

// Conteudo acumulado da cadeia ou do caracter que esta sendo lido, ja com as
// sequencias de escape resolvidas.
//
// Limitacao conhecida: o escape \0 e guardado no meio do conteudo, mas o valor
// entregue no yylval.str e uma string terminada em zero, entao o texto depois
// dele se perde.
static string conteudo;

// Linha onde o literal em leitura comecou, para que o erro de literal nao
// fechado aponte a abertura e nao a quebra de linha que interrompeu.
static int linhaLiteral = 0;

// Traduz a letra que vem depois da barra invertida para o caractere que ela
// representa. Devolve 0 se a sequencia nao existe na linguagem.
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

// Mensagem provisoria de erro lexico. O formato definitivo, com coluna, ainda
// sera padronizado.
static void erroLexicoNaLinha(int linha, const string& mensagem) {
    cerr << "erro lexico na linha " << linha << ": " << mensagem << "\n";
}

static void erroLexico(const string& mensagem) {
    erroLexicoNaLinha(yylineno, mensagem);
}
%}

/*
 * Opções de Configuração do Flex:
 * - noyywrap: Desativa a necessidade de tratar múltiplos arquivos de entrada consecutivamente.
 * - yylineno: Mantém e incrementa a variável global 'yylineno' com o número da linha atual.
 * - nounput: Desativa a função unput() interna do Flex para evitar avisos de código não utilizado.
 * - noinput: Desativa a função input() interna do Flex para evitar avisos de código não utilizado.
 */
%option noyywrap
%option yylineno
%option nounput
%option noinput

/* Estados de inicialização exclusiva (para ler strings e caracteres) */
%x STRING
%x CHAR

/* Definições regulares (atalhos/apelidos para expressões regulares usadas nas regras) */
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
COMENTARIO_BLOCO "/*"([^*]|"*"+[^*/])*"*"+"/"

%%

    /* --- Separadores: nao geram token (docs/gramatica-lexica.md, secao 2) --- */
{ESPACO}             { /* ignora */ }
{NOVA_LINHA}         { /* a contagem de linhas fica por conta do yylineno */ }
{COMENTARIO_LINHA}   { /* ignora */ }
{COMENTARIO_BLOCO}   { /* ignora */ }

    /* --- Palavras reservadas (docs/gramatica-lexica.md, secao 3) ---
     * Precisam vir antes da regra do identificador. Em caso de empate no
     * comprimento, o Flex escolhe a regra declarada primeiro, entao "se" e
     * reconhecido como KW_SE e nao como IDENTIFICADOR. Ja "senaose" casa com
     * {ID} por ser mais longo, que e o comportamento desejado.
     * A linguagem e case sensitive: "Se" e "SE" sao identificadores comuns.
     */
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

    /* --- Identificadores (docs/gramatica-lexica.md, secao 4) ---
     * O lexema vai para o yylval para o parser e a tabela de simbolos usarem.
     */
{ID}                 { yylval.str = duplicarLexema(yytext); return IDENTIFICADOR; }


    /* --- Literais numericos (docs/gramatica-lexica.md, secao 5) ---
     * O sinal negativo nao faz parte do literal: "-5" sao dois tokens.
     */
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

    /* --- Cadeia (docs/gramatica-lexica.md, secao 5.3) ---
     * Lida em estado exclusivo para resolver os escapes e detectar a cadeia
     * que nao foi fechada antes do fim da linha.
     */
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

    /* --- Caracter (docs/gramatica-lexica.md, secao 5.4) --- */
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

    /* --- Provisorio ---
     * Operadores e delimitadores ainda nao tem regra. Ate la, esta regra evita
     * que o Flex aplique o ECHO padrao e copie a entrada para a saida.
     */
.                    { erroLexico(string("caractere inesperado: ") + yytext); }

%%