%{
#include <iostream>
#include <string>
#include "portugol/ast.hpp"

#include "portugol/tabela.hpp"

int yylex();
void yyerror(const char* s);
extern int yynerrs;
extern int yylineno;

static bool tem_funcoes = false;
static bool tem_principal = false;
static int  num_erros_semanticos = 0;

// Tipo da ultima declaracao reduzida em 'tipo:'; usado por 'declarador:'.
static std::string tipoCorrente;
%}

%code requires {
    struct NoAST;
}

%defines
%define parse.error detailed

// O yynerrs e global e nao volta a zero entre chamadas do yyparse. Sem este
// reinicio, a guarda "if (yynerrs > 0) YYABORT" da regra programa faz toda
// analise seguinte a uma que falhou devolver erro, mesmo com programa correto.
%initial-action {
    yynerrs = 0;
    tem_funcoes = false;
    tem_principal = false;
    num_erros_semanticos = 0;
    tipoCorrente.clear();
    portugol::tabela.limpar();
    liberarAST(raizAST);
    raizAST = nullptr;
}

// Tipos de valores semanticos fornecidos pelo Flex (yylval) e nos da AST.
%union {
    char* str;
    int ival;
    double fval;
    char cval;
    const char* op;
    struct NoAST* no;
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

// Tipos dos nao-terminais na AST.
%type <no>   lista_itens item bloco
%type <no>   comando_enquanto comando_para
%type <no>   inicializacao_para_opcional expressao_opcional passo_para_opcional
%type <no>   comando_desvio comando_condicional
%type <no>   declaracao_variavel lista_declaradores declarador inicializacao_opcional
%type <ival> dimensao_opcional
%type <op>   tipo operador_atribuicao
%type <no>   comando_atribuicao acesso expressao
%type <no>   funcao comando_retorno chamada_funcao comando_leia comando_escreva
%type <no>   comando_chamada parametro lista_parametros parametros
%type <no>   lista_argumentos argumentos lista_expressoes_escrita
%type <op>   tipo_retorno

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

// Libera os lexemas e subarvores quando o parser descarta simbolos durante a
// recuperacao de erro. Em producoes normais a posse e transferida para a AST.
%destructor { delete[] $$; } <str>
%destructor { liberarAST($$); } <no>

%start programa

%%

// Aceita programa delimitado ou declaracoes/comandos simples no topo.
programa:
      KW_PROGRAMA ABRE_CHAVE lista_itens FECHA_CHAVE
      {
          if (tem_funcoes && !tem_principal) {
              yyerror("funcao principal() nao definida");
              liberarAST($3);
              raizAST = nullptr;
              YYABORT;
          }
          if (num_erros_semanticos > 0) {
              liberarAST($3);
              raizAST = nullptr;
              YYABORT;
          }
          if (yynerrs > 0) {
              liberarAST($3);
              raizAST = nullptr;
              YYABORT;
          }
          $3->tipo = NO_PROGRAMA;
          raizAST = $3;
      }
    | lista_itens
      {
          if (tem_funcoes && !tem_principal) {
              yyerror("funcao principal() nao definida");
              liberarAST($1);
              raizAST = nullptr;
              YYABORT;
          }
          if (num_erros_semanticos > 0) {
              liberarAST($1);
              raizAST = nullptr;
              YYABORT;
          }
          if (yynerrs > 0) {
              liberarAST($1);
              raizAST = nullptr;
              YYABORT;
          }
          $1->tipo = NO_PROGRAMA;
          raizAST = $1;
      }
    ;

lista_itens:
      %empty
      { $$ = criarNoBloco(); }
    | lista_itens item
      {
          adicionarFilho($1, $2);
          $$ = $1;
      }
    ;

item:
      declaracao_variavel { $$ = $1; }
    | comando_atribuicao  { $$ = $1; }
    | comando_condicional { $$ = $1; }
    | comando_enquanto    { $$ = $1; }
    | comando_para        { $$ = $1; }
    | comando_desvio      { $$ = $1; }
    | comando_retorno     { $$ = $1; }
    | comando_leia        { $$ = $1; }
    | comando_escreva     { $$ = $1; }
    | comando_chamada     { $$ = $1; }
    | funcao              { $$ = $1; }
    | bloco               { $$ = $1; }
    | error PONTO_VIRGULA
      {
          yyerrok;
          $$ = nullptr;
      }
    ;

funcao:
      KW_FUNCAO tipo_retorno IDENTIFICADOR ABRE_PAREN lista_parametros FECHA_PAREN bloco
      {
          tem_funcoes = true;
          if (std::string($3) == "principal") tem_principal = true;
          $$ = criarNoFuncao($2, $3, $5, $7);
          delete[] $3;
      }
    ;

tipo_retorno:
      tipo     { $$ = $1; }
    | KW_VAZIO { $$ = "vazio"; }
    ;

// A lista vazia fica separada da nao vazia para que "f(, inteiro a)" nao seja
// aceito: a virgula so aparece entre dois parametros.
lista_parametros:
      %empty     { $$ = criarNoBloco(); }
    | parametros { $$ = $1; }
    ;

parametros:
      parametro
      {
          $$ = criarNoBloco();
          adicionarFilho($$, $1);
      }
    | parametros VIRGULA parametro
      {
          adicionarFilho($1, $3);
          $$ = $1;
      }
    ;

parametro:
      tipo IDENTIFICADOR
      {
          $$ = criarNoParametro($1, $2);
          delete[] $2;
      }
    ;

comando_retorno:
      KW_RETORNE expressao PONTO_VIRGULA
      { $$ = criarNoRetorne($2); }
    | KW_RETORNE PONTO_VIRGULA
      { $$ = criarNoRetorne(); }
    ;

bloco:
      ABRE_CHAVE lista_itens FECHA_CHAVE
      { $$ = $2; }
    ;

comando_enquanto:
      KW_ENQUANTO ABRE_PAREN expressao FECHA_PAREN bloco
      { $$ = criarNoEnquanto($3, $5); }
    ;

comando_para:
      KW_PARA ABRE_PAREN inicializacao_para_opcional PONTO_VIRGULA expressao_opcional PONTO_VIRGULA passo_para_opcional FECHA_PAREN bloco
      { $$ = criarNoPara($3, $5, $7, $9); }
    ;

inicializacao_para_opcional:
      %empty
      { $$ = nullptr; }
    | acesso operador_atribuicao expressao
      { $$ = criarNoAtribuicao($2, $1, $3); }
    | tipo lista_declaradores
      {
          $2->op = $1;
          $$ = $2;
      }
    ;

expressao_opcional:
      %empty
      { $$ = nullptr; }
    | expressao
      { $$ = $1; }
    ;

passo_para_opcional:
      %empty
      { $$ = nullptr; }
    | acesso operador_atribuicao expressao
      { $$ = criarNoAtribuicao($2, $1, $3); }
    ;

comando_desvio:
      KW_PARE PONTO_VIRGULA
      { $$ = criarNoDesvio(NO_PARE); }
    | KW_CONTINUE PONTO_VIRGULA
      { $$ = criarNoDesvio(NO_CONTINUE); }
    ;

// se (expressao) { ... } senao { ... }
//
// O bloco e obrigatorio nos dois lados, entao o dangling-else nao aparece:
// quando o parser ve o "senao", o bloco anterior ja foi fechado pela chave.
// A terceira alternativa e o encadeamento "senao se", que evita ter de
// aninhar um bloco a cada condicao nova.
comando_condicional:
      KW_SE ABRE_PAREN expressao FECHA_PAREN bloco
      { $$ = criarNoSe($3, $5); }
    | KW_SE ABRE_PAREN expressao FECHA_PAREN bloco KW_SENAO bloco
      { $$ = criarNoSe($3, $5, $7); }
    | KW_SE ABRE_PAREN expressao FECHA_PAREN bloco KW_SENAO comando_condicional
      { $$ = criarNoSe($3, $5, $7); }
    ;

// Leitura: leia(<variavel>);  (<variavel> e um acesso, com ou sem indice)
comando_leia:
      KW_LEIA ABRE_PAREN acesso FECHA_PAREN PONTO_VIRGULA
      { $$ = criarNoLeia($3); }
    ;

// Escrita: escreva(<lista_expressoes>);
// Aceita strings literais, numeros, variaveis, expressoes e chamadas de
// funcao como argumentos, separados por virgula. Pelo menos um argumento
// e obrigatorio (lista_expressoes_escrita nao tem alternativa vazia).
comando_escreva:
      KW_ESCREVA ABRE_PAREN lista_expressoes_escrita FECHA_PAREN PONTO_VIRGULA
      { $$ = criarNoEscreva($3); }
    ;

lista_expressoes_escrita:
      expressao
      {
          $$ = criarNoBloco();
          adicionarFilho($$, $1);
      }
    | lista_expressoes_escrita VIRGULA expressao
      {
          adicionarFilho($1, $3);
          $$ = $1;
      }
    ;

// Declaracoes: inteiro x; real taxa = 1.5; inteiro a, b; inteiro v[10];
declaracao_variavel:
      tipo lista_declaradores PONTO_VIRGULA
      {
          $2->op = $1;
          $$ = $2;
      }
    ;

tipo:
      KW_INTEIRO  { tipoCorrente = "inteiro";  $$ = "inteiro"; }
    | KW_REAL     { tipoCorrente = "real";     $$ = "real"; }
    | KW_CADEIA   { tipoCorrente = "cadeia";   $$ = "cadeia"; }
    | KW_CARACTER { tipoCorrente = "caracter"; $$ = "caracter"; }
    | KW_LOGICO   { tipoCorrente = "logico";   $$ = "logico"; }
    ;

lista_declaradores:
      declarador
      { $$ = criarNoDeclVar("", $1); }
    | lista_declaradores VIRGULA declarador
      {
          adicionarFilho($1, $3);
          $$ = $1;
      }
    ;

declarador:
      IDENTIFICADOR dimensao_opcional inicializacao_opcional
      {
          if (!portugol::tabela.inserirSimbolo($1, tipoCorrente, yylineno)) {
              std::cerr << "erro semantico na linha " << yylineno
                        << ": identificador '" << $1
                        << "' ja declarado no mesmo escopo\n";
              ++num_erros_semanticos;
          }
          $$ = criarNoDeclarador($1, $2, $3);
          delete[] $1;
      }
    ;

dimensao_opcional:
      %empty
      { $$ = -1; }
    | ABRE_COLCHETE NUM_INT FECHA_COLCHETE
      { $$ = $2; }
    ;

inicializacao_opcional:
      %empty
      { $$ = nullptr; }
    | ATRIBUICAO expressao
      { $$ = $2; }
    ;

// Comandos iniciais. Outros comandos ficam para as issues especificas.
comando_atribuicao:
      acesso operador_atribuicao expressao PONTO_VIRGULA
      { $$ = criarNoAtribuicao($2, $1, $3); }
    ;

operador_atribuicao:
      ATRIBUICAO        { $$ = "="; }
    | ATRIB_MAIS        { $$ = "+="; }
    | ATRIB_MENOS       { $$ = "-="; }
    | ATRIB_VEZES       { $$ = "*="; }
    | ATRIB_DIVIDIDO    { $$ = "/="; }
    | ATRIB_RESTO       { $$ = "%="; }
    | ATRIB_BIT_E       { $$ = "&="; }
    | ATRIB_BIT_OU      { $$ = "|="; }
    | ATRIB_BIT_XOU     { $$ = "^="; }
    | ATRIB_DESLOCA_ESQ { $$ = "<<="; }
    | ATRIB_DESLOCA_DIR { $$ = ">>="; }
    ;

acesso:
      IDENTIFICADOR
      {
          $$ = criarNoId($1);
          delete[] $1;
      }
    | IDENTIFICADOR ABRE_COLCHETE expressao FECHA_COLCHETE
      {
          $$ = criarNoAcessoVetor($1, $3);
          delete[] $1;
      }
    ;

// Expressoes usadas em inicializadores e atribuicoes.
expressao:
      NUM_INT                 { $$ = criarNoNum($1); }
    | NUM_REAL                { $$ = criarNoReal($1); }
    | LIT_STRING              { $$ = criarNoTexto($1); delete[] $1; }
    | LIT_CHAR                { $$ = criarNoChar($1); }
    | KW_VERDADEIRO           { $$ = criarNoLogico(true); }
    | KW_FALSO                { $$ = criarNoLogico(false); }
    | acesso                  { $$ = $1; }
    | chamada_funcao          { $$ = $1; }
    | ABRE_PAREN expressao FECHA_PAREN { $$ = $2; }
    | MAIS expressao          %prec UNARIO_MAIS  { $$ = criarNoOpUnaria("+", $2); }
    | MENOS expressao         %prec UNARIO_MENOS { $$ = criarNoOpUnaria("-", $2); }
    | NAO expressao           { $$ = criarNoOpUnaria("!", $2); }
    | BIT_NAO expressao       { $$ = criarNoOpUnaria("~", $2); }
    | expressao OU expressao          { $$ = criarNoOp("||", $1, $3); }
    | expressao E expressao           { $$ = criarNoOp("&&", $1, $3); }
    | expressao BIT_OU expressao      { $$ = criarNoOp("|", $1, $3); }
    | expressao BIT_XOU expressao     { $$ = criarNoOp("^", $1, $3); }
    | expressao BIT_E expressao       { $$ = criarNoOp("&", $1, $3); }
    | expressao IGUAL expressao       { $$ = criarNoOp("==", $1, $3); }
    | expressao DIFERENTE expressao   { $$ = criarNoOp("!=", $1, $3); }
    | expressao MENOR expressao       { $$ = criarNoOp("<", $1, $3); }
    | expressao MENOR_IGUAL expressao { $$ = criarNoOp("<=", $1, $3); }
    | expressao MAIOR expressao       { $$ = criarNoOp(">", $1, $3); }
    | expressao MAIOR_IGUAL expressao { $$ = criarNoOp(">=", $1, $3); }
    | expressao DESLOCA_ESQ expressao { $$ = criarNoOp("<<", $1, $3); }
    | expressao DESLOCA_DIR expressao { $$ = criarNoOp(">>", $1, $3); }
    | expressao MAIS expressao        { $$ = criarNoOp("+", $1, $3); }
    | expressao MENOS expressao       { $$ = criarNoOp("-", $1, $3); }
    | expressao VEZES expressao       { $$ = criarNoOp("*", $1, $3); }
    | expressao DIVIDIDO expressao    { $$ = criarNoOp("/", $1, $3); }
    | expressao RESTO expressao       { $$ = criarNoOp("%", $1, $3); }
    ;

chamada_funcao:
      IDENTIFICADOR ABRE_PAREN lista_argumentos FECHA_PAREN
      {
          $$ = criarNoChamada($1, $3);
          delete[] $1;
      }
    ;

// Chamada usada como comando, descartando o valor: imprimir(); soma(a, b);
comando_chamada:
      chamada_funcao PONTO_VIRGULA
      { $$ = $1; }
    ;

// Mesmo arranjo de lista_parametros: "f(, 1)" nao e aceito.
lista_argumentos:
      %empty     { $$ = criarNoBloco(); }
    | argumentos { $$ = $1; }
    ;

argumentos:
      expressao
      {
          $$ = criarNoBloco();
          adicionarFilho($$, $1);
      }
    | argumentos VIRGULA expressao
      {
          adicionarFilho($1, $3);
          $$ = $1;
      }
    ;

%%

void yyerror(const char* s) {
    extern int yylineno;
    extern char* yytext;
    if (raizAST != nullptr) {
        liberarAST(raizAST);
        raizAST = nullptr;
    }
    std::string tokenInesperado;
    if (yytext == nullptr || yytext[0] == '\0') {
        tokenInesperado = "'fim de arquivo'";
    } else {
        tokenInesperado = std::string("'") + yytext + "'";
    }
    if (s != nullptr && std::string(s).find("syntax error") == std::string::npos && std::string(s) != "erro sintático" && std::string(s) != "Erro sintático") {
        std::cerr << "Erro sintático na linha " << yylineno << ": " << s << "\n";
    } else {
        std::cerr << "Erro sintático na linha " << yylineno << ": inesperado " << tokenInesperado << "\n";
    }
}
