#include "portugol/tabela.hpp"

#include <algorithm>
#include <iostream>

namespace portugol {

TabelaSimbolos tabela;

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
    Simbolo s{nome, tipo, linha, cat, dimensao, tipo_retorno, tipos_parametros};
    pilha_.back().simbolos.push_back(s);
    todos_simbolos_.push_back(s);
    return true;
}

const Simbolo* TabelaSimbolos::buscarSimbolo(const std::string& nome) const {
    for (auto it = pilha_.rbegin(); it != pilha_.rend(); ++it) {
        auto sit = std::find_if(it->simbolos.begin(), it->simbolos.end(),
            [&](const Simbolo& s) { return s.nome == nome; });
        if (sit != it->simbolos.end()) return &*sit;
    }
    return nullptr;
}

const Simbolo* TabelaSimbolos::buscarSimboloEscopoAtual(const std::string& nome) const {
    if (pilha_.empty()) return nullptr;
    const auto& escopo = pilha_.back();
    auto sit = std::find_if(escopo.simbolos.begin(), escopo.simbolos.end(),
        [&](const Simbolo& s) { return s.nome == nome; });
    return sit == escopo.simbolos.end() ? nullptr : &*sit;
}

void TabelaSimbolos::imprimirTabela() const {
    std::cout << "===== Tabela de Simbolos =====\n";
    std::cout << "nome\ttipo\tlinha\n";
    for (const auto& s : todos_simbolos_) {
        std::cout << s.nome << '\t' << s.tipo << '\t' << s.linha << '\n';
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

}