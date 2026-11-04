#ifndef PORTUGOL_AST_HPP
#define PORTUGOL_AST_HPP

#include <string>
#include <vector>

// Tipos de nos da Arvore Sintatica Abstrata (AST), conforme o modelo da
// disciplina (COMP1 - Semana 06) expandido para a gramatica do Portugol.
enum TipoNo {
    NO_PROGRAMA,
    NO_BLOCO,
    NO_DECL_VAR,
    NO_DECLARADOR,
    NO_ATRIBUICAO,
    NO_SE,
    NO_ENQUANTO,
    NO_PARA,
    NO_PARE,
    NO_CONTINUE,
    NO_OP,
    NO_OP_UNARIA,
    NO_NUM,
    NO_REAL,
    NO_STRING,
    NO_CHAR,
    NO_LOGICO,
    NO_ID,
    NO_ACESSO_VETOR
};

// Estrutura de um no da AST:
// - tipo: categoria sintatica do no
// - op: operador ("+", "*", "==", "=", "+=", etc.) ou nome do tipo ("inteiro", "real", ...)
// - nome: identificador de variavel/vetor ou conteudo textual de literal cadeia
// - valor: valor inteiro (NUM_INT), booleano (0/1) ou dimensao de vetor (-1 se escalar)
// - valorReal: valor de ponto flutuante (NUM_REAL)
// - valorChar: caractere literal (LIT_CHAR)
// - esq, dir, extra: ponteiros para nos filhos (expressoes, condicoes, ramos)
// - filhos: lista de instrucoes de um bloco/programa ou declaradores de uma declaracao
struct NoAST {
    TipoNo tipo;
    std::string op;
    std::string nome;
    int valor = 0;
    double valorReal = 0.0;
    char valorChar = '\0';
    NoAST* esq = nullptr;
    NoAST* dir = nullptr;
    NoAST* extra = nullptr;
    std::vector<NoAST*> filhos;
};

// Raiz da AST produzida pela ultima execucao bem-sucedida de yyparse().
extern NoAST* raizAST;

// Construtores de nos para expressoes e literais (modelo COMP1 semana 06)
NoAST* criarNoNum(int valor);
NoAST* criarNoReal(double valor);
NoAST* criarNoTexto(const std::string& texto);
NoAST* criarNoChar(char valor);
NoAST* criarNoLogico(bool valor);
NoAST* criarNoId(const std::string& nome);
NoAST* criarNoAcessoVetor(const std::string& nome, NoAST* indice);
NoAST* criarNoOp(const std::string& op, NoAST* esq, NoAST* dir);
NoAST* criarNoOp(char op, NoAST* esq, NoAST* dir);
NoAST* criarNoOpUnaria(const std::string& op, NoAST* operando);

// Construtores de nos para declaracoes e comandos
NoAST* criarNoDeclarador(const std::string& nome, int tamanhoVetor, NoAST* inicializador);
NoAST* criarNoDeclVar(const std::string& tipoVar, NoAST* primeiroDeclarador = nullptr);
NoAST* criarNoAtribuicao(const std::string& op, NoAST* destino, NoAST* expressao);
NoAST* criarNoBloco(TipoNo tipo = NO_BLOCO);
NoAST* criarNoSe(NoAST* condicao, NoAST* blocoEntao, NoAST* ramoSenao = nullptr);
NoAST* criarNoEnquanto(NoAST* condicao, NoAST* corpo);
NoAST* criarNoPara(NoAST* inicializacao, NoAST* condicao, NoAST* passo, NoAST* corpo);
NoAST* criarNoDesvio(TipoNo tipo);

// Utilitarios de manipulacao, impressao e liberacao da arvore
void adicionarFilho(NoAST* pai, NoAST* filho);
std::string astParaString(const NoAST* raiz, int nivel = 0);
void imprimirAST(const NoAST* raiz, int nivel = 0);
void liberarAST(NoAST* raiz);

#endif  // PORTUGOL_AST_HPP

