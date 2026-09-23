%{
#include <iostream>
#include <string>
#include <cstdlib>

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
// e o scanner precisa definir o yylval que o parser definiria (issue #18).
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
REAL      {DIGITO}+"."{DIGITO}+
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

    /* --- Provisorio ---
     * Literais (issue #4), operadores e delimitadores (issue #5) ainda nao tem
     * regra. Ate la, esta regra evita que o Flex aplique o ECHO padrao e copie
     * a entrada para a saida. O formato definitivo da mensagem de erro, com
     * linha e coluna, e a issue #10.
     */
.                    { cerr << "erro lexico na linha " << yylineno
                            << ": caractere inesperado '" << yytext << "'\n"; }

%%