#include "portugol/semantico.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace portugol {

namespace {

// Tipo usado internamente para expressoes que ja tiveram erro relatado.
// Qualquer verificacao que envolva um operando com este tipo e ignorada, para
// que um unico erro nao gere uma cascata de mensagens.
const std::string TIPO_ERRO = "<erro>";

enum class Categoria { VARIAVEL, VETOR, FUNCAO };

struct Simbolo {
    Categoria categoria;
    std::string tipo;        // tipo da variavel/elemento ou tipo de retorno
    int linha = 0;
    int tamanho = -1;        // dimensao, para vetores
    NoAST* no = nullptr;     // declarador, parametro ou funcao de origem
};

bool numerico(const std::string& t) { return t == "inteiro" || t == "real"; }

// Tipos que podem receber um valor do tipo "origem" (atribuicao, inicializacao,
// passagem de argumento e retorno). Alem do mesmo tipo, aceita as conversoes
// que nao perdem informacao: inteiro -> real e caracter -> cadeia.
bool atribuivel(const std::string& destino, const std::string& origem) {
    return destino == origem ||
           (destino == "real" && origem == "inteiro") ||
           (destino == "cadeia" && origem == "caracter");
}

bool ehZeroLiteral(const NoAST* n) {
    return n != nullptr && ((n->tipo == NO_NUM && n->valor == 0) ||
                            (n->tipo == NO_REAL && n->valorReal == 0.0));
}

// Procura um "pare" que encerre o laco atual, sem descer em lacos internos
// (o "pare" de um laco interno nao interrompe o externo).
bool contemPare(const NoAST* n) {
    if (n == nullptr) return false;
    switch (n->tipo) {
        case NO_PARE: return true;
        case NO_ENQUANTO:
        case NO_PARA:
        case NO_FUNCAO: return false;
        default: break;
    }
    if (contemPare(n->esq) || contemPare(n->dir) || contemPare(n->extra)) return true;
    for (const NoAST* f : n->filhos) {
        if (contemPare(f)) return true;
    }
    return false;
}

// Indica se a execucao de "n" sempre termina em um "retorne".
bool sempreRetorna(const NoAST* n) {
    if (n == nullptr) return false;
    switch (n->tipo) {
        case NO_RETORNE:
            return true;
        case NO_BLOCO:
        case NO_PROGRAMA:
            for (const NoAST* f : n->filhos) {
                if (sempreRetorna(f)) return true;
            }
            return false;
        case NO_SE:
            return n->extra != nullptr && sempreRetorna(n->dir) && sempreRetorna(n->extra);
        case NO_ENQUANTO:
            // "enquanto (verdadeiro)" sem "pare" so sai pelo retorne.
            return n->esq != nullptr && n->esq->tipo == NO_LOGICO && n->esq->valor != 0 &&
                   !contemPare(n->dir);
        default:
            return false;
    }
}

class Analisador {
public:
    explicit Analisador(std::ostream& saida) : saida_(saida) {}

    int executar(NoAST* raiz) {
        if (raiz == nullptr) return 0;
        escopos_.clear();
        abrirEscopo();
        analisarPrograma(raiz);
        fecharEscopo();
        return erros_;
    }

private:
    std::ostream& saida_;
    int erros_ = 0;
    std::vector<std::unordered_map<std::string, Simbolo>> escopos_;
    const NoAST* funcaoAtual_ = nullptr;  // funcao cujo corpo esta sendo analisado
    int profundidadeLaco_ = 0;

    // ---- utilitarios ----------------------------------------------------

    void erro(int linha, const std::string& mensagem) {
        saida_ << "Erro semântico na linha " << linha << ": " << mensagem << "\n";
        ++erros_;
    }

    void abrirEscopo() { escopos_.emplace_back(); }
    void fecharEscopo() { escopos_.pop_back(); }

    Simbolo* buscar(const std::string& nome) {
        for (auto it = escopos_.rbegin(); it != escopos_.rend(); ++it) {
            auto encontrado = it->find(nome);
            if (encontrado != it->end()) return &encontrado->second;
        }
        return nullptr;
    }

    // Insere no escopo corrente; relata redeclaracao se o nome ja existir nele.
    bool declarar(const std::string& nome, const Simbolo& simbolo) {
        auto& escopo = escopos_.back();
        auto existente = escopo.find(nome);
        if (existente != escopo.end()) {
            erro(simbolo.linha, "identificador '" + nome + "' ja declarado neste escopo (linha " +
                                    std::to_string(existente->second.linha) + ")");
            return false;
        }
        escopo.emplace(nome, simbolo);
        return true;
    }

    // ---- programa e funcoes ---------------------------------------------

    void analisarPrograma(NoAST* programa) {
        bool temFuncoes = false;

        // As funcoes sao registradas antes de tudo, entao uma funcao pode
        // chamar outra declarada mais abaixo (e a si mesma, recursivamente).
        for (NoAST* item : programa->filhos) {
            if (item == nullptr || item->tipo != NO_FUNCAO) continue;
            temFuncoes = true;
            Simbolo s{Categoria::FUNCAO, item->op, item->linha, -1, item};
            declarar(item->nome, s);
        }

        for (NoAST* item : programa->filhos) {
            if (item == nullptr) continue;
            if (item->tipo == NO_FUNCAO) {
                analisarFuncao(item);
            } else if (item->tipo == NO_DECL_VAR) {
                analisarComando(item);
            } else if (temFuncoes) {
                erro(item->linha,
                     "comando fora de funcao: quando o programa declara funcoes, apenas "
                     "declaracoes de variaveis globais podem ficar fora delas");
            } else {
                analisarComando(item);
            }
        }
    }

    void analisarFuncao(NoAST* f) {
        if (funcaoAtual_ != nullptr || escopos_.size() > 1) {
            erro(f->linha, "funcao '" + f->nome +
                               "' declarada dentro de outro bloco; funcoes so podem ser "
                               "declaradas no nivel do programa");
            return;
        }

        if (f->nome == "principal") {
            if (!f->filhos.empty()) {
                erro(f->linha, "a funcao principal nao deve receber parametros");
            }
            if (f->op != "vazio" && f->op != "inteiro") {
                erro(f->linha, "a funcao principal deve ser do tipo vazio ou inteiro");
            }
        }

        funcaoAtual_ = f;
        abrirEscopo();
        for (NoAST* p : f->filhos) {
            Simbolo s{Categoria::VARIAVEL, p->op, p->linha, -1, p};
            if (escopos_.back().count(p->nome) != 0) {
                erro(p->linha, "parametro '" + p->nome + "' repetido na funcao '" + f->nome + "'");
            } else {
                declarar(p->nome, s);
            }
        }
        // O corpo compartilha o escopo dos parametros: redeclarar um parametro
        // no corpo e erro (como em C/C++).
        if (f->dir != nullptr) analisarItens(f->dir);
        fecharEscopo();
        funcaoAtual_ = nullptr;

        if (f->op != "vazio" && !sempreRetorna(f->dir)) {
            erro(f->linha, "funcao '" + f->nome + "' do tipo " + f->op +
                               " pode terminar sem 'retorne'");
        }
    }

    // ---- comandos -------------------------------------------------------

    void analisarItens(NoAST* bloco) {
        for (NoAST* item : bloco->filhos) {
            if (item != nullptr) analisarComando(item);
        }
    }

    void analisarBloco(NoAST* bloco) {
        if (bloco == nullptr) return;
        abrirEscopo();
        analisarItens(bloco);
        fecharEscopo();
    }

    void analisarComando(NoAST* n) {
        switch (n->tipo) {
            case NO_DECL_VAR:
                for (NoAST* d : n->filhos) analisarDeclarador(n->op, d);
                break;

            case NO_ATRIBUICAO:
                analisarAtribuicao(n);
                break;

            case NO_SE:
                exigirCondicao(n->esq, "se");
                analisarBloco(n->dir);
                if (n->extra != nullptr) {
                    if (n->extra->tipo == NO_SE) {
                        analisarComando(n->extra);
                    } else {
                        analisarBloco(n->extra);
                    }
                }
                break;

            case NO_ENQUANTO:
                exigirCondicao(n->esq, "enquanto");
                ++profundidadeLaco_;
                analisarBloco(n->dir);
                --profundidadeLaco_;
                break;

            case NO_PARA:
                // A variavel declarada na inicializacao vale no laco inteiro, e
                // o corpo divide o escopo com ela (redeclara-la no corpo e erro).
                abrirEscopo();
                if (n->esq != nullptr) analisarComando(n->esq);
                if (n->dir != nullptr) exigirCondicao(n->dir, "para");
                if (n->extra != nullptr) analisarComando(n->extra);
                ++profundidadeLaco_;
                if (!n->filhos.empty() && n->filhos[0] != nullptr) analisarItens(n->filhos[0]);
                --profundidadeLaco_;
                fecharEscopo();
                break;

            case NO_PARE:
            case NO_CONTINUE:
                if (profundidadeLaco_ == 0) {
                    erro(n->linha, std::string("'") + (n->tipo == NO_PARE ? "pare" : "continue") +
                                       "' so pode ser usado dentro de um laco (enquanto/para)");
                }
                break;

            case NO_RETORNE:
                analisarRetorne(n);
                break;

            case NO_LEIA:
                tipoDestino(n->esq, "leia");
                break;

            case NO_ESCREVA:
                for (NoAST* e : n->filhos) tipoExpressao(e);
                break;

            case NO_CHAMADA:
                analisarChamada(n, false);
                break;

            case NO_FUNCAO:
                analisarFuncao(n);
                break;

            case NO_BLOCO:
                analisarBloco(n);
                break;

            default:
                // Expressoes soltas nao sao produzidas pelo parser como comando.
                tipoExpressao(n);
                break;
        }
    }

    void analisarDeclarador(const std::string& tipo, NoAST* d) {
        d->tipoDado = tipo;
        const bool vetor = d->valor >= 0;

        if (vetor) {
            if (d->valor == 0) {
                erro(d->linha, "tamanho do vetor '" + d->nome + "' deve ser maior que zero");
            }
            if (d->esq != nullptr) {
                erro(d->linha, "vetor '" + d->nome + "' nao pode ser inicializado na declaracao");
                tipoExpressao(d->esq);
            }
        } else if (d->esq != nullptr) {
            // O inicializador e verificado antes de declarar o nome, entao
            // "inteiro x = x;" acusa uso de 'x' nao declarado.
            const std::string t = tipoExpressao(d->esq);
            if (t != TIPO_ERRO && !atribuivel(tipo, t)) {
                erro(d->linha, "nao e possivel inicializar '" + d->nome + "' (" + tipo +
                                   ") com um valor do tipo " + t);
            }
        }

        Simbolo s{vetor ? Categoria::VETOR : Categoria::VARIAVEL, tipo, d->linha, d->valor, d};
        declarar(d->nome, s);
    }

    void exigirCondicao(NoAST* condicao, const char* comando) {
        const std::string t = tipoExpressao(condicao);
        if (t != TIPO_ERRO && t != "logico") {
            erro(condicao->linha, std::string("condicao do '") + comando +
                                      "' deve ser do tipo logico (recebeu " + t + ")");
        }
    }

    void analisarAtribuicao(NoAST* n) {
        const std::string destino = tipoDestino(n->esq, "atribuicao");
        const std::string origem = tipoExpressao(n->dir);
        n->tipoDado = destino;
        if (destino == TIPO_ERRO || origem == TIPO_ERRO) return;

        const std::string& op = n->op;
        const std::string alvo = n->esq->nome;
        bool valido;
        if (op == "=") {
            valido = atribuivel(destino, origem);
        } else if (op == "+=") {
            valido = (destino == "cadeia" && (origem == "cadeia" || origem == "caracter")) ||
                     (numerico(destino) && numerico(origem) && atribuivel(destino, origem));
        } else if (op == "-=" || op == "*=" || op == "/=") {
            valido = numerico(destino) && numerico(origem) && atribuivel(destino, origem);
        } else {
            // %=, &=, |=, ^=, <<=, >>=
            valido = destino == "inteiro" && origem == "inteiro";
        }

        if (!valido) {
            if (op == "=") {
                erro(n->linha, "nao e possivel atribuir um valor do tipo " + origem + " a '" +
                                   alvo + "' (" + destino + ")");
            } else {
                erro(n->linha, "operador '" + op + "' nao se aplica a '" + alvo + "' (" + destino +
                                   ") com um valor do tipo " + origem);
            }
            return;
        }
        if ((op == "/=" || op == "%=") && ehZeroLiteral(n->dir)) {
            erro(n->linha, "divisao por zero");
        }
    }

    void analisarRetorne(NoAST* n) {
        if (funcaoAtual_ == nullptr) {
            // Programa sem funcoes: o corpo do programa funciona como a
            // principal vazio, entao so "retorne;" encerra a execucao.
            if (n->esq != nullptr) {
                tipoExpressao(n->esq);
                erro(n->linha, "'retorne' com valor fora de funcao");
            }
            return;
        }

        const std::string& esperado = funcaoAtual_->op;
        const std::string& nome = funcaoAtual_->nome;
        if (esperado == "vazio") {
            if (n->esq != nullptr) {
                tipoExpressao(n->esq);
                erro(n->linha, "funcao '" + nome + "' e vazio e nao pode retornar valor");
            }
            return;
        }
        if (n->esq == nullptr) {
            erro(n->linha, "funcao '" + nome + "' deve retornar um valor do tipo " + esperado);
            return;
        }
        const std::string t = tipoExpressao(n->esq);
        if (t != TIPO_ERRO && !atribuivel(esperado, t)) {
            erro(n->linha, "retorno da funcao '" + nome + "' deve ser do tipo " + esperado +
                               " (recebeu " + t + ")");
        }
    }

    // Tipo do destino de uma atribuicao ou leitura: variavel escalar ou
    // posicao de vetor. Nao marca o simbolo como lido.
    std::string tipoDestino(NoAST* d, const char* contexto) {
        if (d->tipo == NO_ACESSO_VETOR) {
            return tipoAcessoVetor(d);
        }
        Simbolo* s = buscar(d->nome);
        std::string t = TIPO_ERRO;
        if (s == nullptr) {
            erro(d->linha, "variavel '" + d->nome + "' nao declarada");
        } else if (s->categoria == Categoria::FUNCAO) {
            erro(d->linha, "'" + d->nome + "' e uma funcao e nao pode ser usada como destino de " +
                               contexto);
        } else if (s->categoria == Categoria::VETOR) {
            erro(d->linha, "vetor '" + d->nome + "' deve ser acessado com indice em " + contexto);
        } else {
            t = s->tipo;
        }
        d->tipoDado = t;
        return t;
    }

    // ---- expressoes -----------------------------------------------------

    std::string tipoExpressao(NoAST* e) {
        if (e == nullptr) return TIPO_ERRO;
        std::string t = TIPO_ERRO;
        switch (e->tipo) {
            case NO_NUM: t = "inteiro"; break;
            case NO_REAL: t = "real"; break;
            case NO_STRING: t = "cadeia"; break;
            case NO_CHAR: t = "caracter"; break;
            case NO_LOGICO: t = "logico"; break;
            case NO_ID: t = tipoIdentificador(e); break;
            case NO_ACESSO_VETOR: t = tipoAcessoVetor(e, true); break;
            case NO_OP: t = tipoOperacao(e); break;
            case NO_OP_UNARIA: t = tipoOperacaoUnaria(e); break;
            case NO_CHAMADA: t = analisarChamada(e, true); break;
            default:
                erro(e->linha, "construcao invalida em expressao");
                break;
        }
        e->tipoDado = t;
        return t;
    }

    std::string tipoIdentificador(NoAST* e) {
        Simbolo* s = buscar(e->nome);
        if (s == nullptr) {
            erro(e->linha, "variavel '" + e->nome + "' nao declarada");
            return TIPO_ERRO;
        }
        if (s->categoria == Categoria::FUNCAO) {
            erro(e->linha, "'" + e->nome + "' e uma funcao e deve ser chamada com '(...)'");
            return TIPO_ERRO;
        }
        if (s->categoria == Categoria::VETOR) {
            erro(e->linha, "vetor '" + e->nome + "' deve ser acessado com indice");
            return TIPO_ERRO;
        }
        if (s->no != nullptr) s->no->usado = true;
        return s->tipo;
    }

    std::string tipoAcessoVetor(NoAST* e, bool leitura = false) {
        Simbolo* s = buscar(e->nome);
        const std::string tipoIndice = tipoExpressao(e->esq);
        if (tipoIndice != TIPO_ERRO && tipoIndice != "inteiro") {
            erro(e->linha, "indice do vetor '" + e->nome + "' deve ser do tipo inteiro (recebeu " +
                               tipoIndice + ")");
        }
        if (s == nullptr) {
            erro(e->linha, "vetor '" + e->nome + "' nao declarado");
            return e->tipoDado = TIPO_ERRO;
        }
        if (s->categoria != Categoria::VETOR) {
            erro(e->linha, "'" + e->nome + "' nao e um vetor e nao pode ser indexado");
            return e->tipoDado = TIPO_ERRO;
        }
        if (e->esq != nullptr && e->esq->tipo == NO_NUM &&
            (e->esq->valor < 0 || e->esq->valor >= s->tamanho)) {
            erro(e->linha, "indice " + std::to_string(e->esq->valor) + " fora dos limites do vetor '" +
                               e->nome + "' (tamanho " + std::to_string(s->tamanho) + ")");
        }
        if (leitura && s->no != nullptr) s->no->usado = true;
        return e->tipoDado = s->tipo;
    }

    std::string tipoOperacao(NoAST* e) {
        const std::string a = tipoExpressao(e->esq);
        const std::string b = tipoExpressao(e->dir);
        if (a == TIPO_ERRO || b == TIPO_ERRO) return TIPO_ERRO;

        const std::string& op = e->op;
        std::string resultado = TIPO_ERRO;

        if (op == "+") {
            if (numerico(a) && numerico(b)) {
                resultado = (a == "real" || b == "real") ? "real" : "inteiro";
            } else if ((a == "cadeia" && (b == "cadeia" || b == "caracter")) ||
                       (a == "caracter" && b == "cadeia")) {
                resultado = "cadeia";
            }
        } else if (op == "-" || op == "*" || op == "/") {
            if (numerico(a) && numerico(b)) {
                resultado = (a == "real" || b == "real") ? "real" : "inteiro";
            }
        } else if (op == "%" || op == "&" || op == "|" || op == "^" || op == "<<" || op == ">>") {
            if (a == "inteiro" && b == "inteiro") resultado = "inteiro";
        } else if (op == "==" || op == "!=") {
            if ((numerico(a) && numerico(b)) || a == b) resultado = "logico";
        } else if (op == "<" || op == "<=" || op == ">" || op == ">=") {
            if ((numerico(a) && numerico(b)) || (a == b && (a == "caracter" || a == "cadeia"))) {
                resultado = "logico";
            }
        } else if (op == "&&" || op == "||") {
            if (a == "logico" && b == "logico") resultado = "logico";
        }

        if (resultado == TIPO_ERRO) {
            erro(e->linha, "operador '" + op + "' nao se aplica aos tipos " + a + " e " + b);
            return TIPO_ERRO;
        }
        if ((op == "/" || op == "%") && ehZeroLiteral(e->dir)) {
            erro(e->linha, "divisao por zero");
        }
        return resultado;
    }

    std::string tipoOperacaoUnaria(NoAST* e) {
        const std::string t = tipoExpressao(e->esq);
        if (t == TIPO_ERRO) return TIPO_ERRO;
        const std::string& op = e->op;
        if ((op == "+" || op == "-") && numerico(t)) return t;
        if (op == "!" && t == "logico") return t;
        if (op == "~" && t == "inteiro") return t;
        erro(e->linha, "operador unario '" + op + "' nao se aplica ao tipo " + t);
        return TIPO_ERRO;
    }

    // Verifica uma chamada de funcao. Em expressao, a funcao precisa
    // devolver valor; como comando, o retorno (se houver) e descartado.
    std::string analisarChamada(NoAST* c, bool emExpressao) {
        std::vector<std::string> tiposArgs;
        for (NoAST* arg : c->filhos) tiposArgs.push_back(tipoExpressao(arg));

        Simbolo* s = buscar(c->nome);
        if (s == nullptr) {
            erro(c->linha, "funcao '" + c->nome + "' nao declarada");
            return c->tipoDado = TIPO_ERRO;
        }
        if (s->categoria != Categoria::FUNCAO) {
            erro(c->linha, "'" + c->nome + "' nao e uma funcao");
            return c->tipoDado = TIPO_ERRO;
        }
        if (c->nome == "principal") {
            erro(c->linha, "a funcao principal nao pode ser chamada");
            return c->tipoDado = TIPO_ERRO;
        }

        const NoAST* f = s->no;
        const size_t esperados = f->filhos.size();
        if (tiposArgs.size() != esperados) {
            erro(c->linha, "funcao '" + c->nome + "' espera " + std::to_string(esperados) +
                               " argumento(s), mas recebeu " + std::to_string(tiposArgs.size()));
        } else {
            for (size_t i = 0; i < esperados; ++i) {
                const std::string& tipoParam = f->filhos[i]->op;
                if (tiposArgs[i] != TIPO_ERRO && !atribuivel(tipoParam, tiposArgs[i])) {
                    erro(c->linha, "argumento " + std::to_string(i + 1) + " de '" + c->nome +
                                       "' deve ser do tipo " + tipoParam + " (recebeu " +
                                       tiposArgs[i] + ")");
                }
            }
        }

        if (emExpressao && s->tipo == "vazio") {
            erro(c->linha, "funcao '" + c->nome +
                               "' e vazio e nao devolve valor para ser usado em expressao");
            return c->tipoDado = TIPO_ERRO;
        }
        return c->tipoDado = s->tipo;
    }
};

}  // namespace

int analisarSemantica(NoAST* raiz, std::ostream& erros) {
    Analisador analisador(erros);
    return analisador.executar(raiz);
}

}  // namespace portugol
