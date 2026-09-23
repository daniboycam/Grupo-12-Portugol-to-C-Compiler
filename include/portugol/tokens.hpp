#ifndef PORTUGOL_TOKENS_HPP
#define PORTUGOL_TOKENS_HPP

// Codigos dos tokens usados pelo scanner.
//
// PROVISORIO: enquanto o parser do Bison nao existe, o proprio projeto declara
// os codigos aqui. Quando o Bison passar a gerar PortugolToC.tab.h com os mesmos
// nomes, a ponte de inclusao do portugolToC.l vai preferir aquele cabecalho e
// este arquivo pode ser apagado.
//
// Os codigos comecam em 258 por convencao do Bison: 0 e o fim de arquivo, 1-255
// ficam reservados para caracteres literais e 256/257 para uso interno.

#include <cstring>

// Tipo do valor semantico associado ao token (o mesmo papel do %union do Bison).
// O guarda YYSTYPE_IS_DECLARED e o que o proprio Bison usa, entao quando o
// cabecalho gerado existir esta definicao simplesmente nao entra.
#if !defined(YYSTYPE) && !defined(YYSTYPE_IS_DECLARED)
union YYSTYPE {
    char* str;   ///< lexema de identificadores e conteudo de cadeias (new[], liberado por quem consome)
    int ival;    ///< valor de um literal inteiro
    double fval; ///< valor de um literal real
    char cval;   ///< valor de um literal caracter
};
#define YYSTYPE_IS_DECLARED 1
#endif

// Valor semantico do token que o scanner acabou de reconhecer. O scanner e quem
// define esta variavel enquanto nao ha Bison (ver portugolToC.l).
extern YYSTYPE yylval;

enum TipoToken {
    // Estrutura
    KW_PROGRAMA = 258,
    KW_FUNCAO,
    KW_RETORNE,

    // Tipos
    KW_INTEIRO,
    KW_REAL,
    KW_CADEIA,
    KW_CARACTER,
    KW_LOGICO,
    KW_VAZIO,

    // Condicional
    KW_SE,
    KW_SENAO,

    // Repeticao
    KW_ENQUANTO,
    KW_PARA,
    KW_PARE,
    KW_CONTINUE,

    // Literais logicos
    KW_VERDADEIRO,
    KW_FALSO,

    // Entrada e saida
    KW_LEIA,
    KW_ESCREVA,

    // Identificador de variavel ou funcao
    IDENTIFICADOR,

    // Literais
    NUM_INT,
    NUM_REAL,
    LIT_STRING,
    LIT_CHAR
};

/// Nome legivel de um token, para mensagens de erro e testes.
/// Retorna "DESCONHECIDO" para codigos fora da faixa desta tabela.
inline const char* nomeToken(int token) {
    switch (token) {
        case KW_PROGRAMA:   return "KW_PROGRAMA";
        case KW_FUNCAO:     return "KW_FUNCAO";
        case KW_RETORNE:    return "KW_RETORNE";
        case KW_INTEIRO:    return "KW_INTEIRO";
        case KW_REAL:       return "KW_REAL";
        case KW_CADEIA:     return "KW_CADEIA";
        case KW_CARACTER:   return "KW_CARACTER";
        case KW_LOGICO:     return "KW_LOGICO";
        case KW_VAZIO:      return "KW_VAZIO";
        case KW_SE:         return "KW_SE";
        case KW_SENAO:      return "KW_SENAO";
        case KW_ENQUANTO:   return "KW_ENQUANTO";
        case KW_PARA:       return "KW_PARA";
        case KW_PARE:       return "KW_PARE";
        case KW_CONTINUE:   return "KW_CONTINUE";
        case KW_VERDADEIRO: return "KW_VERDADEIRO";
        case KW_FALSO:      return "KW_FALSO";
        case KW_LEIA:       return "KW_LEIA";
        case KW_ESCREVA:    return "KW_ESCREVA";
        case IDENTIFICADOR: return "IDENTIFICADOR";
        case NUM_INT:       return "NUM_INT";
        case NUM_REAL:      return "NUM_REAL";
        case LIT_STRING:    return "LIT_STRING";
        case LIT_CHAR:      return "LIT_CHAR";
        default:            return "DESCONHECIDO";
    }
}

#endif  // PORTUGOL_TOKENS_HPP
