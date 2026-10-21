#include <cstdio>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>

#include "PortugolToC.tab.hpp"

extern int yylex();
extern int yyparse();
extern int yylineno;
extern char* yytext;
extern FILE* yyin;

namespace {

// Nome legivel do token. A macro evita escrever o nome duas vezes, entao
// acrescentar um token aqui e so repetir a linha com a constante nova.
#define NOME_TOKEN(t) case t: return #t;

const char* nomeToken(int token) {
    switch (token) {
        NOME_TOKEN(IDENTIFICADOR)
        NOME_TOKEN(NUM_INT)
        NOME_TOKEN(NUM_REAL)
        NOME_TOKEN(LIT_STRING)
        NOME_TOKEN(LIT_CHAR)

        NOME_TOKEN(KW_PROGRAMA)
        NOME_TOKEN(KW_FUNCAO)
        NOME_TOKEN(KW_RETORNE)
        NOME_TOKEN(KW_INTEIRO)
        NOME_TOKEN(KW_REAL)
        NOME_TOKEN(KW_CADEIA)
        NOME_TOKEN(KW_CARACTER)
        NOME_TOKEN(KW_LOGICO)
        NOME_TOKEN(KW_VAZIO)
        NOME_TOKEN(KW_SE)
        NOME_TOKEN(KW_SENAO)
        NOME_TOKEN(KW_ENQUANTO)
        NOME_TOKEN(KW_PARA)
        NOME_TOKEN(KW_PARE)
        NOME_TOKEN(KW_CONTINUE)
        NOME_TOKEN(KW_VERDADEIRO)
        NOME_TOKEN(KW_FALSO)
        NOME_TOKEN(KW_LEIA)
        NOME_TOKEN(KW_ESCREVA)

        NOME_TOKEN(MAIS)
        NOME_TOKEN(MENOS)
        NOME_TOKEN(VEZES)
        NOME_TOKEN(DIVIDIDO)
        NOME_TOKEN(RESTO)
        NOME_TOKEN(IGUAL)
        NOME_TOKEN(DIFERENTE)
        NOME_TOKEN(MENOR)
        NOME_TOKEN(MENOR_IGUAL)
        NOME_TOKEN(MAIOR)
        NOME_TOKEN(MAIOR_IGUAL)
        NOME_TOKEN(E)
        NOME_TOKEN(OU)
        NOME_TOKEN(NAO)
        NOME_TOKEN(BIT_E)
        NOME_TOKEN(BIT_OU)
        NOME_TOKEN(BIT_XOU)
        NOME_TOKEN(BIT_NAO)
        NOME_TOKEN(DESLOCA_ESQ)
        NOME_TOKEN(DESLOCA_DIR)

        NOME_TOKEN(ATRIBUICAO)
        NOME_TOKEN(ATRIB_MAIS)
        NOME_TOKEN(ATRIB_MENOS)
        NOME_TOKEN(ATRIB_VEZES)
        NOME_TOKEN(ATRIB_DIVIDIDO)
        NOME_TOKEN(ATRIB_RESTO)
        NOME_TOKEN(ATRIB_BIT_E)
        NOME_TOKEN(ATRIB_BIT_OU)
        NOME_TOKEN(ATRIB_BIT_XOU)
        NOME_TOKEN(ATRIB_DESLOCA_ESQ)
        NOME_TOKEN(ATRIB_DESLOCA_DIR)

        NOME_TOKEN(ABRE_PAREN)
        NOME_TOKEN(FECHA_PAREN)
        NOME_TOKEN(ABRE_CHAVE)
        NOME_TOKEN(FECHA_CHAVE)
        NOME_TOKEN(ABRE_COLCHETE)
        NOME_TOKEN(FECHA_COLCHETE)
        NOME_TOKEN(VIRGULA)
        NOME_TOKEN(PONTO_VIRGULA)

        default: return "DESCONHECIDO";
    }
}

#undef NOME_TOKEN

// O lexema como o programador escreveu. Cadeias e caracteres sao lidos em
// estado exclusivo, caractere a caractere, entao no fim deles o yytext guarda
// so a aspa de fechamento: o texto montado esta no yylval.
std::string lexemaDe(int token) {
    switch (token) {
        case IDENTIFICADOR: {
            // O lexema esta no yytext, mas o lexer alocou uma copia para o
            // parser. Aqui ninguem vai consumi-la, entao liberamos.
            delete[] yylval.str;
            return yytext;
        }
        case LIT_STRING: {
            std::string texto = std::string("\"") + yylval.str + "\"";
            delete[] yylval.str;
            return texto;
        }
        case LIT_CHAR:
            return std::string("'") + yylval.cval + "'";
        default:
            return yytext;
    }
}

// Modo scanner: percorre a entrada chamando o lexer e lista o que ele encontrou.
int listarTokens() {
    std::cout << std::left << std::setw(8) << "LINHA" << std::setw(22) << "TOKEN"
              << "LEXEMA\n";

    int total = 0;
    int token;
    while ((token = yylex()) != 0) {
        const int linha = yylineno;
        const std::string lexema = lexemaDe(token);
        std::cout << std::left << std::setw(8) << linha << std::setw(22)
                  << nomeToken(token) << lexema << "\n";
        ++total;
    }

    std::cout << "\n" << total << " tokens reconhecidos.\n";
    return 0;
}

// Modo padrao: entrega os tokens ao parser.
int analisar() {
    if (yyparse() != 0) {
        std::cerr << "Falha na analise sintatica.\n";
        return 1;
    }
    std::cout << "Analise sintatica concluida com sucesso.\n";
    return 0;
}

void uso(const char* programa) {
    std::cerr << "uso: " << programa << " [opcoes] [arquivo.por]\n\n"
              << "  -t, --tokens   lista os tokens reconhecidos (modo scanner)\n"
              << "  -h, --ajuda    mostra esta mensagem\n\n"
              << "Sem arquivo, le da entrada padrao.\n"
              << "Sem --tokens, executa a analise sintatica.\n";
}

}  // namespace

int main(int argc, char** argv) {
    bool modoScanner = false;
    const char* entrada = nullptr;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "-t" || arg == "--tokens") {
            modoScanner = true;
        } else if (arg == "-h" || arg == "--ajuda") {
            uso(argv[0]);
            return 0;
        } else if (!arg.empty() && arg[0] == '-') {
            std::cerr << "opcao desconhecida: " << arg << "\n\n";
            uso(argv[0]);
            return 2;
        } else if (entrada != nullptr) {
            std::cerr << "erro: informe apenas um arquivo de entrada\n";
            return 2;
        } else {
            entrada = argv[i];
        }
    }

    if (entrada != nullptr) {
        // Sem esta checagem, apontar para um diretorio so produz a mensagem
        // crua do Flex ("input in flex scanner failed").
        if (!std::filesystem::exists(entrada)) {
            std::cerr << "erro: arquivo '" << entrada << "' nao encontrado\n";
            return 2;
        }
        if (!std::filesystem::is_regular_file(entrada)) {
            std::cerr << "erro: '" << entrada << "' nao e um arquivo\n";
            return 2;
        }
        yyin = std::fopen(entrada, "r");
        if (yyin == nullptr) {
            std::cerr << "erro: nao foi possivel abrir '" << entrada << "'\n";
            return 2;
        }
    }

    const int resultado = modoScanner ? listarTokens() : analisar();

    if (yyin != nullptr && entrada != nullptr) {
        std::fclose(yyin);
    }
    return resultado;
}
