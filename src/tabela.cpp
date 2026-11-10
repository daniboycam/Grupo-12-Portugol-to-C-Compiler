#include "portugol/tabela.hpp"

#include <algorithm>
#include <iostream>

namespace portugol {

TabelaSimbolos tabela;

bool TabelaSimbolos::inserirSimbolo(const std::string& nome,
                                    const std::string& tipo,
                                    int linha) {
    if (buscarSimbolo(nome) != nullptr) {
        return false;
    }
    simbolos_.push_back({nome, tipo, linha});
    return true;
}

const Simbolo* TabelaSimbolos::buscarSimbolo(const std::string& nome) const {
    auto it = std::find_if(simbolos_.begin(), simbolos_.end(),
        [&](const Simbolo& s) { return s.nome == nome; });
    return it == simbolos_.end() ? nullptr : &*it;
}

void TabelaSimbolos::imprimirTabela() const {
    std::cout << "===== Tabela de Simbolos =====\n";
    if (simbolos_.empty()) {
        std::cout << "(vazia)\n";
    } else {
        std::cout << "nome\ttipo\tlinha\n";
        for (const auto& s : simbolos_) {
            std::cout << s.nome << "\t" << s.tipo << "\t" << s.linha << "\n";
        }
    }
    std::cout << "==============================\n";
}

void TabelaSimbolos::limpar() {
    simbolos_.clear();
}

std::size_t TabelaSimbolos::tamanho() const {
    return simbolos_.size();
}

} // namespace portugol