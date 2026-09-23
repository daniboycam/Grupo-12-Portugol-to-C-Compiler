%{
#include <iostream>
#include <string>
#include <cstdlib>
using namespace std;
%}

%option noyywrap
%option yylineno
%option nounput
%option noinput

%x STRING
%x CHAR

DIGITO    [0-9]
LETRA     [A-Za-z]
ID        {LETRA}({LETRA}|{DIGITO}|_)*
INTEIRO   {DIGITO}+
REAL      {DIGITO}+"."{DIGITO}+
ESPACO    [ \t]+
NOVA_LINHA \n
COMENTARIO_LINHA "//"[^\n]*
COMENTARIO_BLOCO "/*"([^*]|"*"+[^*/])*"*"+"/"