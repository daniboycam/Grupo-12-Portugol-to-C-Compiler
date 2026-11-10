#include "portugol/ast.hpp"

#include <iostream>
#include <sstream>

NoAST* raizAST = nullptr;

namespace {

std::string indentar(int nivel) {
    if (nivel <= 0) {
        return "";
    }
    return std::string(static_cast<size_t>(nivel) * 2, ' ');
}

std::string escaparChar(char c) {
    switch (c) {
        case '\n': return "\\n";
        case '\t': return "\\t";
        case '\\': return "\\\\";
        case '\'': return "\\'";
        case '\0': return "\\0";
        default:   return std::string(1, c);
    }
}

std::string escaparTexto(const std::string& s) {
    std::string resultado;
    resultado.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '\n': resultado += "\\n"; break;
            case '\t': resultado += "\\t"; break;
            case '\\': resultado += "\\\\"; break;
            case '"':  resultado += "\\\""; break;
            case '\0': resultado += "\\0"; break;
            default:   resultado += c; break;
        }
    }
    return resultado;
}

// Move os filhos de um no-lista para o no de destino e libera so o no-lista,
// sem os filhos, que agora pertencem ao destino.
void absorverFilhos(NoAST* destino, NoAST* lista) {
    if (lista == nullptr) {
        return;
    }
    for (NoAST* filho : lista->filhos) {
        destino->filhos.push_back(filho);
    }
    lista->filhos.clear();
    liberarAST(lista);
}

std::string juntar(const std::vector<NoAST*>& nos, const std::string& separador) {
    std::string resultado;
    for (size_t i = 0; i < nos.size(); ++i) {
        if (i > 0) {
            resultado += separador;
        }
        resultado += astParaString(nos[i], 0);
    }
    return resultado;
}

}  // namespace

NoAST* criarNoNum(int valor) {
    NoAST* no = new NoAST();
    no->tipo = NO_NUM;
    no->valor = valor;
    return no;
}

NoAST* criarNoReal(double valor) {
    NoAST* no = new NoAST();
    no->tipo = NO_REAL;
    no->valorReal = valor;
    return no;
}

NoAST* criarNoTexto(const std::string& texto) {
    NoAST* no = new NoAST();
    no->tipo = NO_STRING;
    no->nome = texto;
    return no;
}

NoAST* criarNoChar(char valor) {
    NoAST* no = new NoAST();
    no->tipo = NO_CHAR;
    no->valorChar = valor;
    return no;
}

NoAST* criarNoLogico(bool valor) {
    NoAST* no = new NoAST();
    no->tipo = NO_LOGICO;
    no->valor = valor ? 1 : 0;
    return no;
}

NoAST* criarNoId(const std::string& nome) {
    NoAST* no = new NoAST();
    no->tipo = NO_ID;
    no->nome = nome;
    return no;
}

NoAST* criarNoAcessoVetor(const std::string& nome, NoAST* indice) {
    NoAST* no = new NoAST();
    no->tipo = NO_ACESSO_VETOR;
    no->nome = nome;
    no->esq = indice;
    return no;
}

NoAST* criarNoOp(const std::string& op, NoAST* esq, NoAST* dir) {
    NoAST* no = new NoAST();
    no->tipo = NO_OP;
    no->op = op;
    no->esq = esq;
    no->dir = dir;
    return no;
}

NoAST* criarNoOp(char op, NoAST* esq, NoAST* dir) {
    return criarNoOp(std::string(1, op), esq, dir);
}

NoAST* criarNoOpUnaria(const std::string& op, NoAST* operando) {
    NoAST* no = new NoAST();
    no->tipo = NO_OP_UNARIA;
    no->op = op;
    no->esq = operando;
    return no;
}

NoAST* criarNoDeclarador(const std::string& nome, int tamanhoVetor, NoAST* inicializador) {
    NoAST* no = new NoAST();
    no->tipo = NO_DECLARADOR;
    no->nome = nome;
    no->valor = tamanhoVetor;
    no->esq = inicializador;
    return no;
}

NoAST* criarNoDeclVar(const std::string& tipoVar, NoAST* primeiroDeclarador) {
    NoAST* no = new NoAST();
    no->tipo = NO_DECL_VAR;
    no->op = tipoVar;
    if (primeiroDeclarador != nullptr) {
        no->filhos.push_back(primeiroDeclarador);
    }
    return no;
}

NoAST* criarNoAtribuicao(const std::string& op, NoAST* destino, NoAST* expressao) {
    NoAST* no = new NoAST();
    no->tipo = NO_ATRIBUICAO;
    no->op = op;
    no->esq = destino;
    no->dir = expressao;
    return no;
}

NoAST* criarNoBloco(TipoNo tipo) {
    NoAST* no = new NoAST();
    no->tipo = tipo;
    return no;
}

NoAST* criarNoSe(NoAST* condicao, NoAST* blocoEntao, NoAST* ramoSenao) {
    NoAST* no = new NoAST();
    no->tipo = NO_SE;
    no->esq = condicao;
    no->dir = blocoEntao;
    no->extra = ramoSenao;
    return no;
}

NoAST* criarNoEnquanto(NoAST* condicao, NoAST* corpo) {
    NoAST* no = new NoAST();
    no->tipo = NO_ENQUANTO;
    no->esq = condicao;
    no->dir = corpo;
    return no;
}

NoAST* criarNoPara(NoAST* inicializacao, NoAST* condicao, NoAST* passo, NoAST* corpo) {
    NoAST* no = new NoAST();
    no->tipo = NO_PARA;
    no->esq = inicializacao;
    no->dir = condicao;
    no->extra = passo;
    if (corpo != nullptr) {
        no->filhos.push_back(corpo);
    }
    return no;
}

NoAST* criarNoDesvio(TipoNo tipo) {
    NoAST* no = new NoAST();
    no->tipo = tipo;
    return no;
}

NoAST* criarNoFuncao(const std::string& tipoRetorno, const std::string& nome,
                     NoAST* parametros, NoAST* corpo) {
    NoAST* no = new NoAST();
    no->tipo = NO_FUNCAO;
    no->op = tipoRetorno;
    no->nome = nome;
    absorverFilhos(no, parametros);
    no->dir = corpo;
    return no;
}

NoAST* criarNoParametro(const std::string& tipo, const std::string& nome) {
    NoAST* no = new NoAST();
    no->tipo = NO_PARAMETRO;
    no->op = tipo;
    no->nome = nome;
    return no;
}

NoAST* criarNoChamada(const std::string& nome, NoAST* argumentos) {
    NoAST* no = new NoAST();
    no->tipo = NO_CHAMADA;
    no->nome = nome;
    absorverFilhos(no, argumentos);
    return no;
}

NoAST* criarNoRetorne(NoAST* expressao) {
    NoAST* no = new NoAST();
    no->tipo = NO_RETORNE;
    no->esq = expressao;
    return no;
}

NoAST* criarNoLeia(NoAST* destino) {
    NoAST* no = new NoAST();
    no->tipo = NO_LEIA;
    no->esq = destino;
    return no;
}

NoAST* criarNoEscreva(NoAST* argumentos) {
    NoAST* no = new NoAST();
    no->tipo = NO_ESCREVA;
    absorverFilhos(no, argumentos);
    return no;
}

void adicionarFilho(NoAST* pai, NoAST* filho) {
    if (pai != nullptr && filho != nullptr) {
        pai->filhos.push_back(filho);
    }
}

std::string astParaString(const NoAST* raiz, int nivel) {
    if (raiz == nullptr) {
        return "";
    }

    const std::string recuo = indentar(nivel);

    switch (raiz->tipo) {
        case NO_NUM:
            return recuo + std::to_string(raiz->valor);

        case NO_REAL: {
            std::ostringstream oss;
            oss << raiz->valorReal;
            return recuo + oss.str();
        }

        case NO_STRING:
            return recuo + "\"" + escaparTexto(raiz->nome) + "\"";

        case NO_CHAR:
            return recuo + "'" + escaparChar(raiz->valorChar) + "'";

        case NO_LOGICO:
            return recuo + (raiz->valor != 0 ? "verdadeiro" : "falso");

        case NO_ID:
            return recuo + raiz->nome;

        case NO_ACESSO_VETOR:
            return recuo + raiz->nome + "[" + astParaString(raiz->esq, 0) + "]";

        case NO_OP:
            return recuo + "(" + raiz->op + " " + astParaString(raiz->esq, 0) +
                   " " + astParaString(raiz->dir, 0) + ")";

        case NO_OP_UNARIA:
            return recuo + "(" + raiz->op + " " + astParaString(raiz->esq, 0) + ")";

        case NO_DECLARADOR: {
            std::string base = raiz->nome;
            if (raiz->valor >= 0) {
                base += "[" + std::to_string(raiz->valor) + "]";
            }
            if (raiz->esq != nullptr) {
                return recuo + "(" + base + " = " + astParaString(raiz->esq, 0) + ")";
            }
            return recuo + base;
        }

        case NO_DECL_VAR: {
            std::ostringstream oss;
            oss << recuo << "(decl " << raiz->op;
            for (size_t i = 0; i < raiz->filhos.size(); ++i) {
                oss << (i == 0 ? " " : ", ") << astParaString(raiz->filhos[i], 0);
            }
            oss << ")";
            return oss.str();
        }

        case NO_ATRIBUICAO:
            return recuo + "(" + raiz->op + " " + astParaString(raiz->esq, 0) +
                   " " + astParaString(raiz->dir, 0) + ")";

        case NO_PARAMETRO:
            return recuo + raiz->op + " " + raiz->nome;

        case NO_CHAMADA:
            if (raiz->filhos.empty()) {
                return recuo + "(chamada " + raiz->nome + ")";
            }
            return recuo + "(chamada " + raiz->nome + " " + juntar(raiz->filhos, " ") + ")";

        case NO_RETORNE:
            if (raiz->esq == nullptr) {
                return recuo + "(retorne)";
            }
            return recuo + "(retorne " + astParaString(raiz->esq, 0) + ")";

        case NO_LEIA:
            return recuo + "(leia " + astParaString(raiz->esq, 0) + ")";

        case NO_ESCREVA:
            return recuo + "(escreva " + juntar(raiz->filhos, " ") + ")";

        case NO_FUNCAO: {
            std::ostringstream oss;
            oss << recuo << "(funcao " << raiz->op << " " << raiz->nome
                << " (" << juntar(raiz->filhos, ", ") << ")\n";
            oss << astParaString(raiz->dir, nivel + 1) << "\n";
            oss << recuo << ")";
            return oss.str();
        }

        case NO_PARE:
            return recuo + "(pare)";

        case NO_CONTINUE:
            return recuo + "(continue)";

        case NO_PROGRAMA:
        case NO_BLOCO: {
            const char* rotulo = (raiz->tipo == NO_PROGRAMA) ? "programa" : "bloco";
            if (raiz->filhos.empty()) {
                return recuo + "(" + rotulo + ")";
            }
            std::ostringstream oss;
            oss << recuo << "(" << rotulo << "\n";
            for (const NoAST* filho : raiz->filhos) {
                oss << astParaString(filho, nivel + 1) << "\n";
            }
            oss << recuo << ")";
            return oss.str();
        }

        case NO_SE: {
            std::ostringstream oss;
            oss << recuo << "(se " << astParaString(raiz->esq, 0) << "\n";
            oss << astParaString(raiz->dir, nivel + 1) << "\n";
            if (raiz->extra != nullptr) {
                oss << indentar(nivel + 1) << "(senao\n";
                oss << astParaString(raiz->extra, nivel + 2) << "\n";
                oss << indentar(nivel + 1) << ")\n";
            }
            oss << recuo << ")";
            return oss.str();
        }

        case NO_ENQUANTO: {
            std::ostringstream oss;
            oss << recuo << "(enquanto " << astParaString(raiz->esq, 0) << "\n";
            oss << astParaString(raiz->dir, nivel + 1) << "\n";
            oss << recuo << ")";
            return oss.str();
        }

        case NO_PARA: {
            std::ostringstream oss;
            const std::string initStr = astParaString(raiz->esq, 0);
            const std::string condStr = astParaString(raiz->dir, 0);
            const std::string passoStr = astParaString(raiz->extra, 0);
            oss << recuo << "(para (" << initStr << "; " << condStr << "; " << passoStr << ")\n";
            if (!raiz->filhos.empty()) {
                oss << astParaString(raiz->filhos[0], nivel + 1) << "\n";
            }
            oss << recuo << ")";
            return oss.str();
        }
    }

    return "";
}

void imprimirAST(const NoAST* raiz, int nivel) {
    if (raiz == nullptr) {
        return;
    }
    std::cout << astParaString(raiz, nivel) << "\n";
}

void liberarAST(NoAST* raiz) {
    if (raiz == nullptr) {
        return;
    }
    liberarAST(raiz->esq);
    liberarAST(raiz->dir);
    liberarAST(raiz->extra);
    for (NoAST* filho : raiz->filhos) {
        liberarAST(filho);
    }
    delete raiz;
}

