#include "portugol/tabela.hpp"

#include <iostream>

namespace portugol {

TabelaSimbolos tabela;

namespace {

const char* nomeCategoria(CategoriaSimbolo categoria) {
    switch (categoria) {
        case CategoriaSimbolo::PARAMETRO: return "parametro";
        case CategoriaSimbolo::FUNCAO: return "funcao";
        default: return "variavel";
    }
}

// Tipo como aparece na impressao: "inteiro[5]" para vetores e a assinatura
// "inteiro(inteiro, real)" para funcoes.
std::string tipoImpresso(const Simbolo& s) {
    if (s.categoria == CategoriaSimbolo::FUNCAO) {
        std::string t = s.tipo_retorno + "(";
        for (std::size_t i = 0; i < s.tipos_parametros.size(); ++i) {
            if (i > 0) t += ", ";
            t += s.tipos_parametros[i];
        }
        return t + ")";
    }
    if (s.dimensao > 0) return s.tipo + "[" + std::to_string(s.dimensao) + "]";
    return s.tipo;
}

}  // namespace

TabelaSimbolos::TabelaSimbolos() {
    entrarEscopo("global");
}

void TabelaSimbolos::entrarEscopo(const std::string& nome) {
    pilha_.push_back(Escopo{nome, {}});
}

void TabelaSimbolos::sairEscopo() {
    if (pilha_.size() > 1) {
        pilha_.pop_back();
    }
}

std::size_t TabelaSimbolos::nivel() const {
    return pilha_.size();
}

void TabelaSimbolos::voltarAoNivel(std::size_t nivel) {
    while (pilha_.size() > nivel && pilha_.size() > 1) {
        pilha_.pop_back();
    }
}

std::string TabelaSimbolos::caminhoEscopoAtual() const {
    if (pilha_.size() <= 1) return "global";
    std::string caminho;
    for (std::size_t i = 1; i < pilha_.size(); ++i) {
        if (i > 1) caminho += "/";
        caminho += pilha_[i].nome;
    }
    return caminho;
}

bool TabelaSimbolos::inserirSimbolo(const std::string& nome,
                                    const std::string& tipo,
                                    int linha,
                                    CategoriaSimbolo cat,
                                    int dimensao,
                                    const std::string& tipo_retorno,
                                    const std::vector<std::string>& tipos_parametros) {
    if (buscarSimboloEscopoAtual(nome) != nullptr) {
        return false;
    }
    todos_simbolos_.push_back(Simbolo{nome, tipo, linha, cat, dimensao, tipo_retorno,
                                      tipos_parametros, caminhoEscopoAtual()});
    pilha_.back().simbolos.push_back(&todos_simbolos_.back());
    return true;
}

const Simbolo* TabelaSimbolos::buscarSimbolo(const std::string& nome) const {
    for (auto escopo = pilha_.rbegin(); escopo != pilha_.rend(); ++escopo) {
        for (const Simbolo* s : escopo->simbolos) {
            if (s->nome == nome) return s;
        }
    }
    return nullptr;
}

const Simbolo* TabelaSimbolos::buscarSimboloEscopoAtual(const std::string& nome) const {
    if (pilha_.empty()) return nullptr;
    for (const Simbolo* s : pilha_.back().simbolos) {
        if (s->nome == nome) return s;
    }
    return nullptr;
}

const std::deque<Simbolo>& TabelaSimbolos::simbolos() const {
    return todos_simbolos_;
}

void TabelaSimbolos::imprimirTabela() const {
    std::cout << "===== Tabela de Simbolos =====\n";
    if (todos_simbolos_.empty()) {
        std::cout << "(vazia)\n";
    } else {
        std::cout << "nome\tcategoria\ttipo\tescopo\tlinha\n";
        for (const auto& s : todos_simbolos_) {
            std::cout << s.nome << '\t' << nomeCategoria(s.categoria) << '\t' << tipoImpresso(s)
                      << '\t' << s.escopo << '\t' << s.linha << '\n';
        }
    }
    std::cout << "==============================\n";
}

void TabelaSimbolos::limpar() {
    pilha_.clear();
    todos_simbolos_.clear();
    entrarEscopo("global");
}

std::size_t TabelaSimbolos::tamanho() const {
    return todos_simbolos_.size();
}

}  // namespace portugol
