%{
#include <iostream>

int yylex();
void yyerror(const char* s);
%}

%defines
%define parse.error detailed

// Tipos de valores semanticos fornecidos pelo Flex (yylval).
%union {
    char* str;
    int ival;
    double fval;
    char cval;
}

// Tokens com valor semantico.
%token <str>  IDENTIFICADOR LIT_STRING
%token <ival> NUM_INT
%token <fval> NUM_REAL
%token <cval> LIT_CHAR

// Palavras reservadas.
%token KW_PROGRAMA KW_FUNCAO KW_RETORNE
%token KW_INTEIRO KW_REAL KW_CADEIA KW_CARACTER KW_LOGICO KW_VAZIO
%token KW_SE KW_SENAO
%token KW_ENQUANTO KW_PARA KW_PARE KW_CONTINUE
%token KW_VERDADEIRO KW_FALSO
%token KW_LEIA KW_ESCREVA

// Operadores.
%token MAIS MENOS VEZES DIVIDIDO RESTO
%token IGUAL DIFERENTE MENOR_IGUAL MENOR MAIOR_IGUAL MAIOR
%token E OU NAO
%token BIT_E BIT_OU BIT_XOU BIT_NAO DESLOCA_ESQ DESLOCA_DIR
%token ATRIBUICAO ATRIB_MAIS ATRIB_MENOS ATRIB_VEZES ATRIB_DIVIDIDO ATRIB_RESTO
%token ATRIB_BIT_E ATRIB_BIT_OU ATRIB_BIT_XOU ATRIB_DESLOCA_ESQ ATRIB_DESLOCA_DIR

// Delimitadores.
%token ABRE_PAREN FECHA_PAREN ABRE_CHAVE FECHA_CHAVE
%token ABRE_COLCHETE FECHA_COLCHETE VIRGULA PONTO_VIRGULA

// Precedencia: do menor nivel para o maior.
%right ATRIBUICAO ATRIB_MAIS ATRIB_MENOS ATRIB_VEZES ATRIB_DIVIDIDO ATRIB_RESTO ATRIB_BIT_E ATRIB_BIT_OU ATRIB_BIT_XOU ATRIB_DESLOCA_ESQ ATRIB_DESLOCA_DIR
%left OU
%left E
%left BIT_OU
%left BIT_XOU
%left BIT_E
%left IGUAL DIFERENTE
%left MENOR MENOR_IGUAL MAIOR MAIOR_IGUAL
%left DESLOCA_ESQ DESLOCA_DIR
%left MAIS MENOS
%left VEZES DIVIDIDO RESTO
%right NAO BIT_NAO UNARIO_MAIS UNARIO_MENOS

// Libera os lexemas alocados pelo lexer quando o parser descarta um token
// durante a recuperacao de erro. Em producoes normais a liberacao e explicita.
%destructor { delete[] $$; } <str>

%start programa

%%

// Aceita programa delimitado ou declaracoes/comandos simples no topo.
programa:
      KW_PROGRAMA ABRE_CHAVE lista_itens FECHA_CHAVE
    | lista_itens
    ;

lista_itens:
      %empty
    | lista_itens item
    ;

item:
      declaracao_variavel
    | comando_atribuicao
    | comando_condicional
    | comando_enquanto
    | comando_para
    | comando_desvio
    | bloco
    ;

bloco:
      ABRE_CHAVE lista_itens FECHA_CHAVE
    ;

comando_enquanto:
      KW_ENQUANTO ABRE_PAREN expressao FECHA_PAREN bloco
    ;

comando_para:
      KW_PARA ABRE_PAREN inicializacao_para_opcional PONTO_VIRGULA expressao_opcional PONTO_VIRGULA passo_para_opcional FECHA_PAREN bloco
    ;

inicializacao_para_opcional:
      %empty
    | acesso operador_atribuicao expressao
    | tipo lista_declaradores
    ;

expressao_opcional:
      %empty
    | expressao
    ;

passo_para_opcional:
      %empty
    | acesso operador_atribuicao expressao
    ;

comando_desvio:
      KW_PARE PONTO_VIRGULA
    | KW_CONTINUE PONTO_VIRGULA
    ;

// se (expressao) { ... } senao { ... }
//
// O bloco e obrigatorio nos dois lados, entao o dangling-else nao aparece:
// quando o parser ve o "senao", o bloco anterior ja foi fechado pela chave.
// A terceira alternativa e o encadeamento "senao se", que evita ter de
// aninhar um bloco a cada condicao nova.
comando_condicional:
      KW_SE ABRE_PAREN expressao FECHA_PAREN bloco
    | KW_SE ABRE_PAREN expressao FECHA_PAREN bloco KW_SENAO bloco
    | KW_SE ABRE_PAREN expressao FECHA_PAREN bloco KW_SENAO comando_condicional
    ;

// Declaracoes: inteiro x; real taxa = 1.5; inteiro a, b; inteiro v[10];
declaracao_variavel:
      tipo lista_declaradores PONTO_VIRGULA
    ;

tipo:
      KW_INTEIRO
    | KW_REAL
    | KW_CADEIA
    | KW_CARACTER
    | KW_LOGICO
    ;

lista_declaradores:
      declarador
    | lista_declaradores VIRGULA declarador
    ;

declarador:
      IDENTIFICADOR dimensao_opcional inicializacao_opcional
      { delete[] $1; }
    ;

dimensao_opcional:
      %empty
    | ABRE_COLCHETE NUM_INT FECHA_COLCHETE
    ;

inicializacao_opcional:
      %empty
    | ATRIBUICAO expressao
    ;

// Comandos iniciais. Outros comandos ficam para as issues especificas.
comando_atribuicao:
      acesso operador_atribuicao expressao PONTO_VIRGULA
    ;

operador_atribuicao:
      ATRIBUICAO
    | ATRIB_MAIS | ATRIB_MENOS | ATRIB_VEZES | ATRIB_DIVIDIDO | ATRIB_RESTO
    | ATRIB_BIT_E | ATRIB_BIT_OU | ATRIB_BIT_XOU
    | ATRIB_DESLOCA_ESQ | ATRIB_DESLOCA_DIR
    ;

acesso:
      IDENTIFICADOR
      { delete[] $1; }
    | IDENTIFICADOR ABRE_COLCHETE expressao FECHA_COLCHETE
      { delete[] $1; }
    ;

// Expressoes usadas em inicializadores e atribuicoes.
expressao:
      NUM_INT
    | NUM_REAL
    | LIT_STRING              { delete[] $1; }
    | LIT_CHAR
    | KW_VERDADEIRO
    | KW_FALSO
    | acesso
    | ABRE_PAREN expressao FECHA_PAREN
    | MAIS expressao          %prec UNARIO_MAIS
    | MENOS expressao         %prec UNARIO_MENOS
    | NAO expressao
    | BIT_NAO expressao
    | expressao OU expressao
    | expressao E expressao
    | expressao BIT_OU expressao
    | expressao BIT_XOU expressao
    | expressao BIT_E expressao
    | expressao IGUAL expressao
    | expressao DIFERENTE expressao
    | expressao MENOR expressao
    | expressao MENOR_IGUAL expressao
    | expressao MAIOR expressao
    | expressao MAIOR_IGUAL expressao
    | expressao DESLOCA_ESQ expressao
    | expressao DESLOCA_DIR expressao
    | expressao MAIS expressao
    | expressao MENOS expressao
    | expressao VEZES expressao
    | expressao DIVIDIDO expressao
    | expressao RESTO expressao
    ;

%%

void yyerror(const char* s) {
    extern int yylineno;
    std::cerr << "erro sintatico na linha " << yylineno << ": " << s << "\n";
}
