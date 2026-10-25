%{
#include <iostream>
#include <string>

int yylex();
void yyerror(const char* s);
%}

%defines

// Tipos de valores semanticos (yylval)
%union {
    char* str;
    int ival;
    double fval;
    char cval;
}

// Tokens com valor semantico
%token <str>  IDENTIFICADOR LIT_STRING
%token <ival> NUM_INT
%token <fval> NUM_REAL
%token <cval> LIT_CHAR

// Palavras reservadas
%token KW_PROGRAMA KW_FUNCAO KW_RETORNE
%token KW_INTEIRO KW_REAL KW_CADEIA KW_CARACTER KW_LOGICO KW_VAZIO
%token KW_SE KW_SENAO
%token KW_ENQUANTO KW_PARA KW_PARE KW_CONTINUE
%token KW_VERDADEIRO KW_FALSO
%token KW_LEIA KW_ESCREVA

// Operadores
%token MAIS MENOS VEZES DIVIDIDO RESTO
%token IGUAL DIFERENTE MENOR_IGUAL MENOR MAIOR_IGUAL MAIOR
%token E OU NAO
%token BIT_E BIT_OU BIT_XOU BIT_NAO DESLOCA_ESQ DESLOCA_DIR
%token ATRIBUICAO ATRIB_MAIS ATRIB_MENOS ATRIB_VEZES ATRIB_DIVIDIDO ATRIB_RESTO
%token ATRIB_BIT_E ATRIB_BIT_OU ATRIB_BIT_XOU ATRIB_DESLOCA_ESQ ATRIB_DESLOCA_DIR

// Delimitadores
%token ABRE_PAREN FECHA_PAREN ABRE_CHAVE FECHA_CHAVE
%token ABRE_COLCHETE FECHA_COLCHETE VIRGULA PONTO_VIRGULA

// Precedencia e associatividade de operadores
%right ATRIBUICAO ATRIB_MAIS ATRIB_MENOS ATRIB_VEZES ATRIB_DIVIDIDO ATRIB_RESTO ATRIB_BIT_E ATRIB_BIT_OU ATRIB_BIT_XOU ATRIB_DESLOCA_ESQ ATRIB_DESLOCA_DIR
%left  OU
%left  E
%left  BIT_OU
%left  BIT_XOU
%left  BIT_E
%left  IGUAL DIFERENTE
%left  MENOR MENOR_IGUAL MAIOR MAIOR_IGUAL
%left  DESLOCA_ESQ DESLOCA_DIR
%left  MAIS MENOS
%left  VEZES DIVIDIDO RESTO
%right NAO BIT_NAO

%%

programa:
    /* vazio por enquanto */
    { }
    ;

%%

void yyerror(const char* s) {
    extern int yylineno;
    std::cerr << "erro sintatico na linha " << yylineno << ": " << s << "\n";
}
