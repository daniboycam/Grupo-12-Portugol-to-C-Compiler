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
#endif

using namespace std;
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
ID        {LETRA}({LETRA}|{DIGITO}|_)*
INTEIRO   {DIGITO}+
REAL      {DIGITO}+"."{DIGITO}+
ESPACO    [ \t]+
NOVA_LINHA \n
COMENTARIO_LINHA "//"[^\n]*
COMENTARIO_BLOCO "/*"([^*]|"*"+[^*/])*"*"+"/"

%%
	/* Seção de regras léxicas: os padrões e tokens serão inseridos nas próximas issues */
%%